#ifndef QUERY_PLAN_H_
#define QUERY_PLAN_H_

#include <iostream>
#include <string>
#include <vector>

#include "DBFile.h"
#include "Schema.h"
#include "Function.h"
#include "ParseTree.h"
#include "Statistics.h"
#include "Comparison.h"

#define MAX_RELS 12
#define MAX_RELNAME 50
#define MAX_ATTS 100

class TreeNode;
class QueryMaker {
public:
  QueryMaker(Statistics* st);
  ~QueryMaker() { if (root) delete root; }

  void plan();
  void print(std::ostream& os = std::cout) const;
  void setOutput(char* out);
  void execute();

private:
  void makeLeafs();
  void makeJoins();
  void orderJoins();
  void makeSums();
  void makeProjects();
  void makeDistinct();
  void makeWrite();
  int evalOrder(std::vector<TreeNode*> operands, Statistics st, int bestFound); // intentional copy

  TreeNode* root;
  std::vector<TreeNode*> nodes;
  std::string outName;
  FILE* outFile;

  Statistics* stat;
  AndList* used;  // reconstruct AndList so that it can be used next time
                  // should be assigned to boolean after each round
  void recycleList(AndList* alist) { concatList(used, alist); }
  static void concatList(AndList*& left, AndList*& right);

  QueryMaker(const QueryMaker&);
  QueryMaker& operator=(const QueryMaker&);
};

class Pipe; class RelationalOp;
class TreeNode {
  friend class QueryMaker;
  friend class UnaryNode;
  friend class BinaryNode;   // passed as argument to binary node
  friend class ProjectNode;
  friend class DedupNode;
  friend class JoinNode;
  friend class SumNode;
  friend class GroupByNode;
  friend class WriteNode;
public:
  virtual ~TreeNode();

protected:
  TreeNode(const std::string& op, Schema* out, Statistics* st);
  TreeNode(const std::string& op, Schema* out, char* rName, Statistics* st);
  TreeNode(const std::string& op, Schema* out, char* rNames[], size_t num, Statistics* st);

  virtual void print(std::ostream& os = std::cout, size_t level = 0) const;
  virtual void printOperator(std::ostream& os = std::cout, size_t level = 0) const;
  virtual void printSchema(std::ostream& os = std::cout, size_t level = 0) const;
  virtual void printAnnot(std::ostream& os = std::cout, size_t level = 0) const = 0; // operator specific
  virtual void printPipe(std::ostream& os, size_t level = 0) const = 0;
  virtual void printChildren(std::ostream& os, size_t level = 0) const = 0;

  virtual void execute(Pipe** pipes, RelationalOp** relops) = 0;

  static AndList* pushSelection(AndList*& alist, Schema* target);
  static bool containedIn(OrList* ors, Schema* target);
  static bool containedIn(ComparisonOp* cmp, Schema* target);

  std::string opName;
  Schema* outSchema;
  char* relNames[MAX_RELS];
  size_t numRels;
  int estimate, cost;  // estimated number of tuples and total cost
  Statistics* stat;
  int pout;  // output pipe
  static int pipeId;
};

class FileNode: private TreeNode {  // read from file
  friend class QueryMaker;
  FileNode (AndList*& boolean, AndList*& pushed,
            char* relName, char* alias, Statistics* st);
  ~FileNode() { if (opened) dbf.Close(); }
  void printOperator(std::ostream& os = std::cout, size_t level = 0) const;
  void printAnnot(std::ostream& os = std::cout, size_t level = 0) const;
  void printPipe(std::ostream& os, size_t level) const;
  void printChildren(std::ostream& os, size_t level) const {}

  void execute(Pipe** pipes, RelationalOp** relops);

  DBFile dbf;
  bool opened;
  CNF selOp;
  Record literal;
};

class UnaryNode: protected TreeNode {
  friend class QueryMaker;
protected:
  UnaryNode(const std::string& opName, Schema* out, TreeNode* c, Statistics* st);
  virtual ~UnaryNode() { delete child; }
  void printPipe(std::ostream& os, size_t level) const;
  void printChildren(std::ostream& os, size_t level) const { child->print(os, level+1); }
  TreeNode* child;
  int pin;  // input pipe
};

class BinaryNode: protected TreeNode {  // not including set operations.
  friend class QueryMaker;
protected:
  BinaryNode(const std::string& opName, TreeNode* l, TreeNode* r, Statistics* st);
  virtual ~BinaryNode() { delete left; delete right; }
  void printPipe(std::ostream& os, size_t level) const;
  void printChildren(std::ostream& os, size_t level) const
  { left->print(os, level+1); right->print(os, level+1); }
  TreeNode* left;
  TreeNode* right;
  int pleft, pright; // input pipes
};

class ProjectNode: private UnaryNode {
  friend class QueryMaker;
  ProjectNode(NameList* atts, TreeNode* c);
  void printAnnot(std::ostream& os = std::cout, size_t level = 0) const;
  void execute(Pipe** pipes, RelationalOp** relops);
  int keepMe[MAX_ATTS];
  int numAttsIn, numAttsOut;
};

class DedupNode: private UnaryNode {
  friend class QueryMaker;
  DedupNode(TreeNode* c);
  void printAnnot(std::ostream& os = std::cout, size_t level = 0) const {}
  void execute(Pipe** pipes, RelationalOp** relops);
  OrderMaker dedupOrder;
};

class SumNode: private UnaryNode {
  friend class QueryMaker;
  SumNode(FuncOperator* parseTree, TreeNode* c);
  Schema* resultSchema(FuncOperator* parseTree, TreeNode* c);
  void printAnnot(std::ostream& os = std::cout, size_t level = 0) const;
  void execute(Pipe** pipes, RelationalOp** relops);
  Function f;
};

class GroupByNode: private UnaryNode {
  friend class QueryMaker;
  GroupByNode(NameList* gAtts, FuncOperator* parseTree, TreeNode* c);
  Schema* resultSchema(NameList* gAtts, FuncOperator* parseTree, TreeNode* c);
  void printAnnot(std::ostream& os = std::cout, size_t level = 0) const;
  void execute(Pipe** pipes, RelationalOp** relops);
  OrderMaker grpOrder;
  Function f;
};

class JoinNode: private BinaryNode {
  friend class QueryMaker;
  JoinNode(AndList*& boolean, AndList*& pushed, TreeNode* l, TreeNode* r, Statistics* st);
  void printAnnot(std::ostream& os = std::cout, size_t level = 0) const;
  void execute(Pipe** pipes, RelationalOp** relops);
  CNF selOp;
  Record literal;
};

class WriteNode: private UnaryNode {
  friend class QueryMaker;
  WriteNode(FILE*& out, TreeNode* c);
  void printAnnot(std::ostream& os = std::cout, size_t level = 0) const;
  void execute(Pipe** pipes, RelationalOp** relops);
  FILE*& outFile;
};

#endif

