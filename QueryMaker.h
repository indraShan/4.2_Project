#ifndef QueryMaker_H
#define QueryMaker_H
#include "Statistics.h"
#include <vector>
#include "ParseTree.h"
#include "Schema.h"

class TreeNode {
private:
    

public:
    char* displayString;
    Schema *schema;
    int estimatedCost;
    char *relNames[20];
    size_t numberOfRelations;
    TreeNode(char* displayString, Schema *schema);
    TreeNode(char* displayString, Schema *schema, char *relName);
    ~TreeNode();
    void print();
    TreeNode *left;
    TreeNode *right;
    virtual void printSelf();
};


// Represent the leaves of the tree.
// Read/write using DBFile and Sorted file.
class FileNode: public TreeNode {
private:
    TableList *table;
public:
    FileNode(TableList *table, char *alias, Schema *schema, char *relName);
    void printSelf();
};

class ProjectNode: public TreeNode {
public:
ProjectNode(TreeNode *root, NameList *attsToSelect);
};

class JoinNode: public TreeNode {
private:

public:
    JoinNode(TreeNode *node1, TreeNode* node2);
};


class DuplicateRemovalNode: public TreeNode {
public:
    DuplicateRemovalNode(TreeNode *root);
};

class SumNode: public TreeNode {
public:
    Schema* constructSchemaFrom(TreeNode *root, FuncOperator *finalFunction);
    SumNode(TreeNode *root, FuncOperator *finalFunction);
};

class GroupByNode: public TreeNode {
public:
GroupByNode(TreeNode *root, NameList *groupingAtts, FuncOperator *finalFunction);
Schema* constructSchemaFrom(TreeNode *root, NameList *groupingAtts, FuncOperator *finalFunction);
};

class WriteOutNode: public TreeNode {
public:
    WriteOutNode(TreeNode *root);
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

public:
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
        char *catalogPath); 
    virtual ~QueryMaker();

    void make();
    void printQuery();
};
#endif