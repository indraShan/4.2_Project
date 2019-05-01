#include <iostream>
#include <stdlib.h> 
#include "QueryMaker.h"
#include <algorithm>
#include <climits>
#include <stdlib.h>
#include <string.h>

static int PipeId = 0;

TreeNode::TreeNode(char* displayString, Schema *schema)
: pipeId(PipeId++), estimatedCost(0), numberOfRelations(0), left(NULL), right(NULL) {
    this->schema = schema;
    this->displayString = displayString;
}

TreeNode::TreeNode(char* displayString, Schema *schema, char *relName)
 : pipeId(PipeId++), schema(schema), displayString(displayString), estimatedCost(0), left(NULL), right(NULL) {
    this->numberOfRelations = 1;
    this->relNames[0] = relName;
}

TreeNode::~TreeNode() {

}

string TreeNode::makeIndentation(int depth) {
    string indent = "";
    for (int index = 0; index < depth; index++) {
        indent += "\t";
    }
    indent += "->";
    return indent;
}

// Inorder?
void TreeNode::print(int depth) {
        // Print this node's data here.
    string indetation = makeIndentation(depth);
    printf("%s %s Operation", indetation.c_str(), this->displayString);
    printSelf();
    printf(". PipeId = %d \n", pipeId);
    if (this->left != NULL) {
        // printf("%s left of %s \n", indetation.c_str(), this->displayString);
        this->left->print(depth + 1);
    }

    if (this->right != NULL) {
        // printf("%s right of %s\n", indetation.c_str(), this->displayString);
        this->right->print(depth + 1);
    }
}

void TreeNode::run(vector<RelationalOp *> *operators) {
    
}

void ProjectNode::run(vector<RelationalOp *> *operators) {
    if (this->left != NULL) {
        this->left->run(operators);
    }
    Project *projectOperator = new Project();
    pipe = new Pipe(100);
    projectOperator->Run(*this->left->pipe, *pipe, attributesKept, this->numAttsInput, attsToKeep);
    operators->push_back(projectOperator);
}

void WriteOutNode::run(vector<RelationalOp *> *operators) {
    printf("WriteOut run called \n");
    if (this->left != NULL) {
        this->left->run(operators);
    }
    // WriteOut *writeOutOperator = new WriteOut();
    // writeOutOperator->Run(*this->left->pipe, this->outputFile, *this->schema);
    // operators->push_back(writeOutOperator);
}


void SumNode::run(vector<RelationalOp *> *operators) {
    printf("SumNode run called \n");
    if (this->left != NULL) {
        this->left->run(operators);
    }
    // Sum *sumOperator = new Sum();
    // pipe = new Pipe(100);
    // sumOperator->Run(*this->left->pipe, *pipe, *this->fnc);
    // operators->push_back(sumOperator);
}

void GroupByNode::run(vector<RelationalOp *> *operators) {
    printf("GroupByNode run called \n");
    if (this->left != NULL) {
        this->left->run(operators);
    }  
    GroupBy *groupByOperation = new GroupBy();
    pipe = new Pipe(100);
    groupByOperation->Run(*this->left->pipe, *pipe, om, *fnc);
    operators->push_back(groupByOperation);
}

void JoinNode::run(vector<RelationalOp *> *operators) {
    printf("JoinNode run called \n");
    if (this->left != NULL) {
        this->left->run(operators);
    }
    if (this->right != NULL) {
        this->right->run(operators);
    }
    Join *joinOperator = new Join();
    pipe = new Pipe(100);
    joinOperator->Run(*this->left->pipe, *this->right->pipe, *pipe, this->cnf, rec);
    operators->push_back(joinOperator);
}

void FileNode::run(vector<RelationalOp *> *operators) {
    printf("FileNode run called \n");
    databaseFile = new DBFile();
    string fileName = string(relNames[0]) + DBFileExtension;
    // printf("Open called \n");
    databaseFile->Open((char *)fileName.c_str());
    SelectFile *selectOperation = new SelectFile();
    pipe = new Pipe(100);
    printf("Will call run \n");
    selectOperation->Run(*databaseFile, *pipe, this->cnf, rec);
    operators->push_back(selectOperation);
}

void TreeNode::printSelf() {
//   printAnnot(os, level);
//   printSchema(os, level);
//   printPipe(os, level);
}


QueryMaker::QueryMaker(
    FuncOperator *finalFunction, 
    TableList *tables, AndList *boolean,
    NameList *groupingAtts, 
    NameList *attsToSelect, 
    int distinctAtts, 
    int distinctFunc, 
    Statistics *statistics, 
    char *catalogPath, 
    string output) {

    this->finalFunction = finalFunction;
    this->tables = tables;
    this->boolean = boolean;
    this->groupingAtts = groupingAtts;
    this->attsToSelect = attsToSelect;
    this->distinctAtts = distinctAtts;
    this->distinctFunc = distinctFunc;
    this->statistics = statistics;
    this->root = NULL;
    this->nodes = new vector<TreeNode*>();
    this->catolog_path = catalogPath;
    this->output = output;
}

QueryMaker::~QueryMaker () {

}

void QueryMaker::printQuery() {
    // printf("Print called \n");
    if (this->root == NULL) {
        return;
    }
    this->root->print(0);
}

FileNode::FileNode(TableList *table, char *alias, Schema *schema, char *relName) : TreeNode("File", schema, relName) {
    // printf("FileNode called \n");
    this->table = table;
    this->schema->setAlias(alias);
}

void FileNode::printSelf() {
    this->cnf.Print();
}

void JoinNode::printSelf() {
    this->cnf.Print();
}

void SumNode::printSelf() {
    this->fnc->Print();
}

void GroupByNode::printSelf() {
    this->om.PrintInLine();
    this->fnc->Print();
}

void ProjectNode::printSelf() {
    printf(" Atts to keep : %d | ", attsToKeep);
    for(int i = 0; i < attsToKeep; i++){
        printf("%s - ",attributes[i].name);
        if(attributes[i].myType == Int){
            printf("Int");
        }
        else if(attributes[i].myType == Double){
            printf("Double");
        }
        else printf("String");
        if(i < attsToKeep - 1){
            printf(", ");
        }
    }
}

void QueryMaker::runQuery() {
    
    printf("Run query called. \n");
    operators = new vector<RelationalOp *>();
    this->root->run(operators);

     for (std::vector<RelationalOp*>::iterator runsIterator = operators->begin(); runsIterator != operators->end(); ++runsIterator)
	{
		RelationalOp *relOperator = *runsIterator;
		relOperator->WaitUntilDone();
	}
}

void QueryMaker::make() {
    // Make tables.
    AndList *leastCostTree = NULL;
    for (TableList* table = tables; table; table = table->next) {
        this->statistics->CopyRel(table->tableName, table->aliasAs);
        Schema *sch = new Schema(this->catolog_path, table->tableName);
        FileNode *node = new FileNode(table, table->aliasAs, sch, table->tableName);
        node->cnf.GrowFromParseTree(buildParseTreeAfterApplyingSelect(boolean, sch), sch, node->rec);
        this->nodes->push_back(node);
    }
    // // If there are multiple tables, find the most efficient join order.
    int minJoinCost = INT_MAX;
    // printf("Nodes size = %d \n", nodes->size());
    vector<TreeNode *> intermediate(*nodes);
    // printf("intermediate size = %d \n", intermediate.size());
    // sort to make next_permutation happy
    sort(intermediate.begin(), intermediate.end());
    do
    { 
        std::pair<int, AndList *> pair = evaluateJoinedParseTreeForOrder(intermediate, minJoinCost);
        if (pair.first < minJoinCost) {
            minJoinCost = pair.first;
            leastCostTree = pair.second;
        } 
    } while (next_permutation(intermediate.begin(), intermediate.end()));

    this->root = nodes->front();

    // printf("After creating joins. size = %d \n", nodes->size());

    if (groupingAtts) {
        if (distinctFunc) {
            root = new DuplicateRemovalNode(root);
        }
        root = new GroupByNode(root, groupingAtts, finalFunction);
    }
    else if (finalFunction) {
        root = new SumNode(root, finalFunction);
    }

    if (attsToSelect && !finalFunction && !groupingAtts) {
        root = new ProjectNode(root, attsToSelect);
    }

    if (distinctAtts) {
        root = new DuplicateRemovalNode(root);
    }

    root = new WriteOutNode(root, this->output);
}

WriteOutNode::WriteOutNode(TreeNode *root, string output) : TreeNode("Write", new Schema(root->schema)) {
    // printf("Write node called \n");
    this->left = root;
    this->outputFile = NULL;
    if (output == "STDOUT") {
        this->outputFile = stdout;
    }
    else if (output != "NONE") {
        this->outputFile = fopen(output.c_str(), "w");
    }

    for (size_t index = 0; index < root->numberOfRelations; index++) {
        this->relNames[this->numberOfRelations++] = strdup(root->relNames[index]);
    }
}

ProjectNode::ProjectNode(TreeNode *root, NameList *attsToSelect) : TreeNode("Project", NULL)  {
    // printf("ProjectNode called \n");
    this->left = root;
    this->numAttsInput = root->schema->GetNumAtts();
    for (size_t index = 0; index < root->numberOfRelations; index++) {
        this->relNames[this->numberOfRelations++] = strdup(root->relNames[index]);
    }
    Schema *rootSchema = root->schema;
    int index = 0;
    for (; attsToSelect; attsToSelect = attsToSelect->next, index++) {
        attributes[index].name = attsToSelect->name;
        attributes[index].myType = rootSchema->FindType(attsToSelect->name);
        attributesKept[index] = rootSchema->Find(attsToSelect->name);
        attsToKeep++;
    }
    schema = new Schema("", index, attributes);
}

SumNode::SumNode(TreeNode *root, FuncOperator *finalFunction) : TreeNode("SumNode", constructSchemaFrom(root, finalFunction)) {
    // printf("SumNode called \n");
    this->left = root;
    for (size_t index = 0; index < root->numberOfRelations; index++) {
        this->relNames[this->numberOfRelations++] = strdup(root->relNames[index]);
    }
}

Schema* SumNode::constructSchemaFrom(TreeNode *root, FuncOperator *finalFunction) {
  Schema *rootSchema = root->schema;
    fnc = new Function();
  fnc->GrowFromParseTree (finalFunction, *rootSchema);
  Attribute atts[2][1] = {{{"sum", Int}}, {{"sum", Double}}};
  return new Schema ("", 1, atts[fnc->returnsInt ? Int : Double]);
}

GroupByNode::GroupByNode(TreeNode *root, NameList *groupingAtts, FuncOperator *finalFunction) : TreeNode("GroupBy", constructSchemaFrom(root, groupingAtts, finalFunction)) {
    // printf("GroupByNode called \n");
    this->left = root;
    om.constructOrderMaker(groupingAtts, this->schema);
    for (size_t index = 0; index < root->numberOfRelations; index++) {
        this->relNames[this->numberOfRelations++] = strdup(root->relNames[index]);
    }
}

Schema* GroupByNode::constructSchemaFrom(TreeNode *root, NameList *groupingAtts, FuncOperator *finalFunction) {
    Function function;
    Schema *rootSchema = root->schema;
    fnc = new Function();
    fnc->GrowFromParseTree(finalFunction, *rootSchema);
    Attribute attributes[20];
    attributes[0].name = "sum"; 
    attributes[0].myType = fnc->returnsInt ? Int : Double;
    int numberOfAttributes = 1;
    for (; groupingAtts; groupingAtts = groupingAtts->next, numberOfAttributes++)
    {
        attributes[numberOfAttributes].name = groupingAtts->name; 
        attributes[numberOfAttributes].myType = rootSchema->FindType(groupingAtts->name);
    }
    return new Schema("", numberOfAttributes, attributes);
}

DuplicateRemovalNode::DuplicateRemovalNode(TreeNode *root) : TreeNode("DuplicateRemoval", new Schema(root->schema)) {
    // printf("DuplicateRemovalNode called \n");
    this->left = root;
    for (size_t index = 0; index < root->numberOfRelations; index++) {
        this->relNames[this->numberOfRelations++] = strdup(root->relNames[index]);
    }
}

JoinNode::JoinNode(TreeNode *node1, TreeNode* node2) : TreeNode("Join", new Schema(node1->schema, node2->schema)) {
    // printf("JoinNode called \n");
    this->left = node1;
    this->right = node2;
    for (size_t index = 0; index < node1->numberOfRelations; index++) {
        this->relNames[this->numberOfRelations++] = strdup(node1->relNames[index]);
    }
    for (size_t index = 0; index < node2->numberOfRelations; index++) {
        this->relNames[this->numberOfRelations++] = strdup(node2->relNames[index]);
    }
}

AndList* QueryMaker::buildParseTreeAfterApplyingSelect(AndList *&source, Schema *schema) {
  AndList *builtTree = NULL;
  AndList copied; 
  copied.rightAnd = source; 

  AndList *current = source;
  AndList *previous = &copied;
  for (; current; current = previous->rightAnd) {
    if (attributesPresentInSchema(current->left, schema)) {
      previous->rightAnd = current->rightAnd;
      current->rightAnd = builtTree;
      builtTree = current;
    } else {
        previous = current;
    }
  }
  source = copied.rightAnd;
  return builtTree;
}

bool QueryMaker::attributesPresentInSchema(OrList *orList, Schema *schema) {
  for (; orList; orList = orList->rightOr) {
      if (!operationMatchesSchema(orList->left, schema)) return false;
  }
  return true;
}

bool QueryMaker::operationMatchesSchema(ComparisonOp *operation, Schema *schema) {
  return (operation->left->code != NAME || schema->Find(operation->left->value) != -1) && 
  (operation->right->code != NAME || schema->Find(operation->right->value) != -1);
}

void QueryMaker::append(AndList*& source, AndList*& addition) {
    if (source == NULL) {
        swap(source, addition);
        return;
    }
    AndList *previous = source;
    AndList *current = source->rightAnd;
    for (; current; previous = current, current = current->rightAnd);
    previous->rightAnd = addition;
    addition = NULL;
}

// Build a parse tree that matches the passed in order.
// The cost of this tree is the addition of cost of each joined node.
// TODO: Review cost logic.
pair<int, AndList *> QueryMaker::evaluateJoinedParseTreeForOrder(vector<TreeNode *> order,
 int cureentMin) {
     bool failure = false;
     AndList *builtTree = NULL;
     int cost = 0;

// printf("Eval called with size = %d \n", order.size());
//  for (std::vector<TreeNode*>::iterator runsIterator = order.begin(); runsIterator != order.end(); ++runsIterator)
// 	{
// 		TreeNode *node = *runsIterator;
// 		node->print();
// 	}

     while (order.size() > 1) {
         // Join the last two nodes
         TreeNode *node1 = order.back();
         order.pop_back(); 
         TreeNode *node2 = order.back(); 
         order.pop_back();

        // printf("Size after removal = %d \n", order.size());
        JoinNode *joined = new JoinNode(node1, node2);
        order.push_back(joined);
        // printf("Size after adding join = %d \n", order.size());

        AndList *currentTree = buildParseTreeAfterApplyingSelect(boolean, joined->schema);
        joined->cnf.GrowFromParseTree(currentTree, node1->schema, node2->schema, joined->rec);
        append(builtTree, currentTree);
        
        int currentCost = this->statistics->Estimate(currentTree, joined->relNames, joined->numberOfRelations);
        // printf("currentCost = %d \n", currentCost);
        if (currentCost < 0) {
            // printf("Breaking out \n");
            // TODO: handle failure
            failure = true;
            break;
        }
        cost += currentCost;
        if (cost > cureentMin) {
            failure = true;
            // printf("Breaking out 2 \n");
            break;
        }
     }
     if (failure == false && cost < cureentMin) {
        //  printf("Prnting the only tree in order \n");
        //  order.front()->print();
        //  printf("size of the array =%d \n", order.size());
         nodes = new vector<TreeNode*>(order);
     }
    //  printf("cost = %d \n", cost);
     if (failure) {
         return std::make_pair(INT_MAX, builtTree);
     }

     return std::make_pair(cost, builtTree);
}