#include <cstring>
#include <climits>
#include <string>
#include <algorithm>
#include "Defs.h"
#include "QueryMaker.h"
#include "Pipe.h"
#include "RelOp.h"

#define indent(level) (string(3*(level), ' ') + "-> ")
#define annot(level) (string(3*(level+1), ' ') + "* ")

using std::endl;
using std::string;

extern char* catalog_path;
extern char* dbfile_dir;
extern char* tpch_dir;

// from parser
extern FuncOperator* finalFunction;
extern TableList* tables;
extern AndList* boolean;
extern NameList* groupingAtts;
extern NameList* attsToSelect;
extern int distinctAtts;
extern int distinctFunc;


/**********************************************************************
 * API                                                                *
 **********************************************************************/
QueryMaker::QueryMaker(Statistics* st): root(NULL), outName("STDOUT"), stat(st), used(NULL) {}

void QueryMaker::plan() {
  for (TableList* table = tables; table; table = table->next) {
    stat->CopyRel(table->tableName, table->aliasAs);
    AndList* pushed; 
    FileNode* newLeaf = new FileNode (boolean, pushed, table->tableName, table->aliasAs, stat); 
    concatList(used, pushed);
    nodes.push_back(newLeaf);
  }
 
  orderJoins();
  while (nodes.size()>1) {
    TreeNode *left = nodes.back();                 
    nodes.pop_back();                               
    TreeNode *right = nodes.back();                  
    nodes.pop_back();
    AndList* pushed; 
    JoinNode* newJoinNode = new JoinNode (boolean, pushed, left, right, stat); 
    concatList(used, pushed);
    nodes.push_back(newJoinNode);
  }
  root = nodes.front();

  if (groupingAtts) {
    if (distinctFunc) root = new DedupNode(root);
    root = new GroupByNode(groupingAtts, finalFunction, root);
  } else if (finalFunction) {
    root = new SumNode(finalFunction, root);
  }

  if (attsToSelect && !finalFunction && !groupingAtts) root = new ProjectNode(attsToSelect, root);

  if (distinctAtts) root = new DedupNode(root);

  root = new WriteNode(outFile, root);
}

void QueryMaker::print(std::ostream& os) const {
  root->print(os);
}

void QueryMaker::setOutput(char* out) {
  outName = out;
}

void QueryMaker::execute() {
  outFile = (outName == "STDOUT" ? stdout
    : outName == "NONE" ? NULL
    : fopen(outName.c_str(), "w"));   // closed by query executor
  if (outFile) {
    int numNodes = root->pipeId;
    Pipe** pipes = new Pipe*[numNodes];
    RelationalOp** relops = new RelationalOp*[numNodes];
    root->execute(pipes, relops);
    for (int i=0; i<numNodes; ++i)
      relops[i] -> WaitUntilDone();
    for (int i=0; i<numNodes; ++i) {
      delete pipes[i]; delete relops[i];
    }
    delete[] pipes; delete[] relops;
    if (outFile!=stdout) fclose(outFile);
  }
  root->pipeId = 0;
  delete root; root = NULL;
  nodes.clear();
}

void QueryMaker::orderJoins() {
  std::vector<TreeNode*> operands(nodes);
  sort(operands.begin(), operands.end());
  int minCost = INT_MAX, cost;
  do {           // traverse all possible permutations
    if ((cost=evalOrder(operands, *stat, minCost))<minCost && cost>0) {
      minCost = cost; nodes = operands; 
    }
  } while (next_permutation(operands.begin(), operands.end()));
}

int QueryMaker::evalOrder(std::vector<TreeNode*> operands, Statistics st, int bestFound) {  // intentional copy
  std::vector<JoinNode*> freeList;  // all new nodes made in this simulation; need to be freed
  AndList* recycler = NULL;         // AndList needs recycling
  while (operands.size()>1) {       // simulate join
    TreeNode *left = operands.back();                 
    operands.pop_back();                               
    TreeNode *right = operands.back();                  
    operands.pop_back();

  AndList* pushed; 
  JoinNode* newJoinNode = new JoinNode (boolean, pushed, left, right, &st); 
  concatList(recycler, pushed);
    operands.push_back(newJoinNode);
    freeList.push_back(newJoinNode);
    if (newJoinNode->estimate<=0 || newJoinNode->cost>bestFound) break;  // branch and bound
  }
  int cost = operands.back()->cost;
  return operands.back()->estimate<0 ? -1 : cost;
}

void QueryMaker::concatList(AndList*& left, AndList*& right) {
  if (!left) { swap(left, right); return; }
  AndList *pre = left, *cur = left->rightAnd;
  for (; cur; pre = cur, cur = cur->rightAnd);
  pre->rightAnd = right;
  right = NULL;
}

/**********************************************************************
 * Node construction                                                  *
 **********************************************************************/
int TreeNode::pipeId = 0;

TreeNode::TreeNode(const std::string& op, Schema* out, Statistics* st):
  opName(op), outSchema(out), numRels(0), estimate(0), cost(0), stat(st), pout(pipeId++) {}

TreeNode::TreeNode(const std::string& op, Schema* out, char* rName, Statistics* st):
  opName(op), outSchema(out), numRels(0), estimate(0), cost(0), stat(st), pout(pipeId++) {
  if (rName) relNames[numRels++] = strdup(rName);
}

TreeNode::TreeNode(const std::string& op, Schema* out, char* rNames[], size_t num, Statistics* st):
  opName(op), outSchema(out), numRels(0), estimate(0), cost(0), stat(st), pout(pipeId++) {
  for (; numRels<num; ++numRels)
    relNames[numRels] = strdup(rNames[numRels]);
}

TreeNode::~TreeNode() {
  delete outSchema;
  for (size_t i=0; i<numRels; ++i)
    delete[] relNames[i];
}

AndList* TreeNode::pushSelection(AndList*& alist, Schema* target) {
  AndList header; header.rightAnd = alist;  // make a list header to
  // avoid handling special cases deleting the first list element
  AndList *cur = alist, *pre = &header, *result = NULL;
  for (; cur; cur = pre->rightAnd)
    if (containedIn(cur->left, target)) {   // should push
      pre->rightAnd = cur->rightAnd;
      cur->rightAnd = result;        // *move* the node to the result list
      result = cur;        // prepend the new node to result list
    } else pre = cur;
  alist = header.rightAnd;  // special case: first element moved
  return result;
}

bool TreeNode::containedIn(OrList* ors, Schema* target) {
  for (; ors; ors=ors->rightOr)
    if (!containedIn(ors->left, target)) return false;
  return true;
}

bool TreeNode::containedIn(ComparisonOp* cmp, Schema* target) {
  Operand *left = cmp->left, *right = cmp->right;
  return (left->code!=NAME || target->Find(left->value)!=-1) &&
         (right->code!=NAME || target->Find(right->value)!=-1);
}

    // AndList* pushed; 
    // FileNode* newLeaf = new FileNode (boolean, pushed, table->tableName, table->aliasAs, stat);
    // concatList(used, pushed);
FileNode::FileNode(AndList*& boolean, AndList*& pushed, 
char* relName, char* alias, Statistics* st):
  TreeNode("Select File", new Schema(catalog_path, relName), relName, st), opened(false) {
      outSchema->setAlias(alias);
  pushed = pushSelection(boolean, outSchema);
  estimate = stat->ApplyEstimate(pushed, relNames, numRels);
  selOp.GrowFromParseTree(pushed, outSchema, literal);
}

UnaryNode::UnaryNode(const std::string& opName, Schema* out, TreeNode* c, Statistics* st):
  TreeNode (opName, out, c->relNames, c->numRels, st), child(c), pin(c->pout) {}

BinaryNode::BinaryNode(const std::string& opName, TreeNode* l, TreeNode* r, Statistics* st):
  TreeNode (opName, new Schema(l->outSchema, r->outSchema), st),
  left(l), right(r), pleft(left->pout), pright(right->pout) {
  for (size_t i=0; i<l->numRels;)
    relNames[numRels++] = strdup(l->relNames[i++]);
  for (size_t j=0; j<r->numRels;)
    relNames[numRels++] = strdup(r->relNames[j++]);
}

ProjectNode::ProjectNode(NameList* atts, TreeNode* c):
  UnaryNode("Project", NULL, c, NULL), numAttsIn(c->outSchema->GetNumAtts()), numAttsOut(0) {
  Schema* cSchema = c->outSchema;
  Attribute resultAtts[MAX_ATTS];
//   FATALIF (cSchema->GetNumAtts()>MAX_ATTS, "Too many attributes.");
  for (; atts; atts=atts->next, numAttsOut++) {
    resultAtts[numAttsOut].name = atts->name; 
    resultAtts[numAttsOut].myType = cSchema->FindType(atts->name);
  }
  outSchema = new Schema ("", numAttsOut, resultAtts);
}

DedupNode::DedupNode(TreeNode* c):
  UnaryNode("Deduplication", new Schema(*c->outSchema), c, NULL), dedupOrder(c->outSchema) {}

JoinNode::JoinNode(AndList*& boolean, AndList*& pushed, TreeNode* l, TreeNode* r, Statistics* st):
  BinaryNode("Join", l, r, st) {
  pushed = pushSelection(boolean, outSchema);
  estimate = stat->ApplyEstimate(pushed, relNames, numRels);
  cost = l->cost + estimate + r->cost;
  selOp.GrowFromParseTree(pushed, l->outSchema, r->outSchema, literal);
}

SumNode::SumNode(FuncOperator* parseTree, TreeNode* c):
  UnaryNode("Sum", resultSchema(parseTree, c), c, NULL) {
  f.GrowFromParseTree (parseTree, *c->outSchema);
}

Schema* SumNode::resultSchema(FuncOperator* parseTree, TreeNode* c) {
  Function fun;
  Attribute atts[2][1] = {{{"sum", Int}}, {{"sum", Double}}};
  fun.GrowFromParseTree (parseTree, *c->outSchema);
  return new Schema ("", 1, atts[fun.returnsInt ? Int : Double]);
}

GroupByNode::GroupByNode(NameList* gAtts, FuncOperator* parseTree, TreeNode* c):
  UnaryNode("Group by", resultSchema(gAtts, parseTree, c), c, NULL) {
  grpOrder.growFromParseTree(gAtts, c->outSchema);
  f.GrowFromParseTree (parseTree, *c->outSchema);
}

Schema* GroupByNode::resultSchema(NameList* gAtts, FuncOperator* parseTree, TreeNode* c) {
  Function fun;
  Schema* cSchema = c->outSchema;
  fun.GrowFromParseTree (parseTree, *cSchema);
  Attribute resultAtts[MAX_ATTS];
//   FATALIF (1+cSchema->GetNumAtts()>MAX_ATTS, "Too many attributes.");
  resultAtts[0].name = "sum"; 
  resultAtts[0].myType = fun.returnsInt ? Int : Double;
  int numAtts = 1;
  for (; gAtts; gAtts=gAtts->next, numAtts++) {
    // FATALIF (cSchema->Find(gAtts->name)==-1, "Grouping by non-existing attribute.");
    resultAtts[numAtts].name = gAtts->name; 
    resultAtts[numAtts].myType = cSchema->FindType(gAtts->name);
  }
  return new Schema ("", numAtts, resultAtts);
}

WriteNode::WriteNode(FILE*& out, TreeNode* c):
  UnaryNode("WriteOut", new Schema(*c->outSchema), c, NULL), outFile(out) {}


/**********************************************************************
 * Query execution                                                    *
 **********************************************************************/
void FileNode::execute(Pipe** pipes, RelationalOp** relops) {
  std::string dbName = std::string(relNames[0]) + ".bin";
  dbf.Open((char*)dbName.c_str()); opened = true;
  SelectFile* sf = new SelectFile();
  pipes[pout] = new Pipe(512);
  relops[pout] = sf;
  sf -> Run(dbf, *pipes[pout], selOp, literal);
}

void ProjectNode::execute(Pipe** pipes, RelationalOp** relops) {
  child -> execute(pipes, relops);
  Project* p = new Project();
  pipes[pout] = new Pipe(512);
  relops[pout] = p;
  p -> Run(*pipes[pin], *pipes[pout], keepMe, numAttsIn, numAttsOut);
}

void DedupNode::execute(Pipe** pipes, RelationalOp** relops) {
  child -> execute(pipes, relops);
  DuplicateRemoval* dedup = new DuplicateRemoval();
  pipes[pout] = new Pipe(512);
  relops[pout] = dedup;
  dedup -> Run(*pipes[pin], *pipes[pout], *outSchema);
}

void SumNode::execute(Pipe** pipes, RelationalOp** relops) {
  child -> execute(pipes, relops);
  Sum* s = new Sum();
  pipes[pout] = new Pipe(512);
  relops[pout] = s;
  s -> Run(*pipes[pin], *pipes[pout], f);
}

void GroupByNode::execute(Pipe** pipes, RelationalOp** relops) {
  child -> execute(pipes, relops);
  GroupBy* grp = new GroupBy();
  pipes[pout] = new Pipe(512);
  relops[pout] = grp;
  grp -> Run(*pipes[pin], *pipes[pout], grpOrder, f);
}

void JoinNode::execute(Pipe** pipes, RelationalOp** relops) {
  left -> execute(pipes, relops); right -> execute(pipes, relops);
  Join* j = new Join();
  pipes[pout] = new Pipe(512);
  relops[pout] = j;
  j -> Run(*pipes[pleft], *pipes[pright], *pipes[pout], selOp, literal);
}

void WriteNode::execute(Pipe** pipes, RelationalOp** relops) {
  child -> execute(pipes, relops);
  WriteOut* w = new WriteOut();
  pipes[pout] = new Pipe(512);
  relops[pout] = w;
  w -> Run(*pipes[pin], outFile, *outSchema);
}

/**********************************************************************
 * Print utilities                                                    *
 **********************************************************************/
void TreeNode::print(std::ostream& os, size_t level) const {
  printOperator(os, level);
  printAnnot(os, level);
  printSchema(os, level);
  printPipe(os, level);
  printChildren(os, level);
}

void TreeNode::printOperator(std::ostream& os, size_t level) const {
  os << indent(level) << opName << ": ";
}

void TreeNode::printSchema(std::ostream& os, size_t level) const {
#ifdef _OUTPUT_SCHEMA__
  os << annot(level) << "Output schema:" << endl;
  outSchema->print(os);
#endif
}

void FileNode::printPipe(std::ostream& os, size_t level) const {
  os << annot(level) << "Output pipe: " << pout << endl;
}

void UnaryNode::printPipe(std::ostream& os, size_t level) const {
  os << annot(level) << "Output pipe: " << pout << endl;
  os << annot(level) << "Input pipe: " << pin << endl;
}

void BinaryNode::printPipe(std::ostream& os, size_t level) const {
  os << annot(level) << "Output pipe: " << pout << endl;
  os << annot(level) << "Input pipe: " << pleft << ", " << pright << endl;
}

void FileNode::printOperator(std::ostream& os, size_t level) const {
  os << indent(level) << "Select from " << relNames[0] << ": ";
}

void FileNode::printAnnot(std::ostream& os, size_t level) const {
//   selOp.Print(); 
}

void ProjectNode::printAnnot(std::ostream& os, size_t level) const {
  os << keepMe[0];
  for (size_t i=1; i<numAttsOut; ++i) os << ',' << keepMe[i];
  os << endl;
  os << annot(level) << numAttsIn << " input attributes; " << numAttsOut << " output attributes" << endl;
}

void JoinNode::printAnnot(std::ostream& os, size_t level) const {
//   selOp.Print();
  os << annot(level) << "Estimate = " << estimate << ", Cost = " << cost << endl;
}

void SumNode::printAnnot(std::ostream& os, size_t level) const {
  os << annot(level) << "Function: "; (const_cast<Function*>(&f))->Print();
}

void GroupByNode::printAnnot(std::ostream& os, size_t level) const {
  os << annot(level) << "OrderMaker: "; (const_cast<OrderMaker*>(&grpOrder))->Print();
  os << annot(level) << "Function: "; (const_cast<Function*>(&f))->Print();
}

void WriteNode::printAnnot(std::ostream& os, size_t level) const {
  os << annot(level) << "Output to " << outFile << endl;
}

