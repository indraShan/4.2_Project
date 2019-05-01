#ifndef QueryMaker_H
#define QueryMaker_H
#include "Statistics.h"
#include <vector>
#include "ParseTree.h"
#include "Schema.h"
#include "Function.h"
#include "DBFile.h"
#include "RelOp.h"

class TreeNode {
private:
    

public:
    int pipeId;
    Pipe *pipe;
    char* displayString;
    Schema *schema;
    int estimatedCost;
    char *relNames[20];
    size_t numberOfRelations;
    TreeNode(char* displayString, Schema *schema);
    string makeIndentation(int depth);
    TreeNode(char* displayString, Schema *schema, char *relName);
    ~TreeNode();
    void print(int depth);
    TreeNode *left;
    TreeNode *right;
    virtual void run(vector<RelationalOp *> *operators);
    virtual void printSelf();
};


// Represent the leaves of the tree.
// Read/write using DBFile and Sorted file.
class FileNode: public TreeNode {
private:
    TableList *table;
    DBFile *databaseFile;
public:
    CNF cnf;
    Record rec;
    FileNode(TableList *table, char *alias, Schema *schema, char *relName);
    void run(vector<RelationalOp *> *operators);
    void printSelf();
};

class ProjectNode: public TreeNode {
int numAttsInput;
public:
Attribute attributes[100];
int attsToKeep;
int attributesKept[100];
void printSelf();
void run(vector<RelationalOp *> *operators);
ProjectNode(TreeNode *root, NameList *attsToSelect);
};

class JoinNode: public TreeNode {
private:

public:
    void run(vector<RelationalOp *> *operators);
    CNF cnf;
    Record rec;
    void printSelf();
    JoinNode(TreeNode *node1, TreeNode* node2);
};


class DuplicateRemovalNode: public TreeNode {
public:
    DuplicateRemovalNode(TreeNode *root);
};

class SumNode: public TreeNode {
public:
    Function *fnc;
    Schema* constructSchemaFrom(TreeNode *root, FuncOperator *finalFunction);
    void printSelf();
    SumNode(TreeNode *root, FuncOperator *finalFunction);
    void run(vector<RelationalOp *> *operators);
};

class GroupByNode: public TreeNode {
public:
    Function *fnc;
    OrderMaker om;
    GroupByNode(TreeNode *root, NameList *groupingAtts, FuncOperator *finalFunction);
    void printSelf();
    void run(vector<RelationalOp *> *operators);
    Schema* constructSchemaFrom(TreeNode *root, NameList *groupingAtts, FuncOperator *finalFunction);
};

class WriteOutNode: public TreeNode {
public:
    FILE *outputFile;
    WriteOutNode(TreeNode *root, string output);
    void run(vector<RelationalOp *> *operators);
};

class QueryMaker {
private:
FuncOperator *finalFunction; 
TableList *tables;
AndList *boolean;
NameList *groupingAtts;
NameList *attsToSelect;
int distinctAtts;
int distinctFunc;
Statistics *statistics;
vector<TreeNode*> *nodes;
char *catolog_path;

pair<int, AndList *> evaluateJoinedParseTreeForOrder(vector<TreeNode *> order, int cureentMin);
AndList* buildParseTreeAfterApplyingSelect(AndList *&source, Schema *schema);
bool operationMatchesSchema(ComparisonOp *operation, Schema *schema);
bool attributesPresentInSchema(OrList *orList, Schema *schema);
void append(AndList*& source, AndList*& addition);
vector<RelationalOp *> *operators;
string output;

public:
    void runQuery();
    TreeNode *root;
    QueryMaker (
        FuncOperator *finalFunction, 
        TableList *tables,
        AndList *boolean,
        NameList *groupingAtts,
        NameList *attsToSelect, 
        int distinctAtts, 
        int distinctFunc, 
        Statistics *statistics, 
        char *catalogPath, 
        string output); 
    virtual ~QueryMaker();

    void make();
    void printQuery();
};
#endif
