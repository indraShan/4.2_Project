#include <iostream>
#include <stdlib.h> 
#include "QueryMaker.h"
#include <algorithm>
#include <climits>
#include <stdlib.h>
#include <string.h>
#include "Function.h"

TreeNode::TreeNode(char* displayString, Schema *schema) {
    this->schema = schema;
    this->displayString = displayString;
}

TreeNode::TreeNode(char* displayString, Schema *schema, char *relName) : displayString(displayString),  schema(schema) {
    this->estimatedCost = 0;
    this->numberOfRelations = 1;
    this->relNames[0] = relName;
}

TreeNode::~TreeNode() {

}

void TreeNode::print() {
    if (this->left != NULL) {
        this->left->print();
    }
    // Print this node's data here.
    printf(this->displayString);
    printf(" Operation \n");
    printSelf();

    if (this->right != NULL) {
        this->right->print();
    }
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
    char *catalogPath) {

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
}

QueryMaker::~QueryMaker () {

}

void QueryMaker::printQuery() {
    if (this->root == NULL) {
        return;
    }
    this->root->print();
}

FileNode::FileNode(TableList *table, char *alias, Schema *schema, char *relName) : TreeNode("File", schema, relName) {
    printf("FileNode called \n");
    this->table = table;
    this->schema->setAlias(alias);
}

void FileNode::printSelf() {
    // printf("Table name =%s, aliasAs = %s \n", table->tableName, table->aliasAs);
}

void QueryMaker::make() {
    // Make tables.
    AndList *leastCostTree = NULL;
    for (TableList* table = tables; table; table = table->next) {
        this->statistics->CopyRel(table->tableName, table->aliasAs);
        FileNode *node = new FileNode(table, table->aliasAs, new Schema(this->catolog_path, table->tableName), table->tableName);
        this->nodes->push_back(node);
    }
    // // If there are multiple tables, find the most efficient join order.
    int minJoinCost = INT_MAX;
    printf("Nodes size = %d \n", nodes->size());
    vector<TreeNode *> intermediate(*nodes);
    printf("intermediate size = %d \n", intermediate.size());
    // sort to make next_permutation happy
    sort(intermediate.begin(), intermediate.end());
    do
    { 
        std::pair<int, AndList *> pair = evaluateJoinedParseTreeForOrder(intermediate, minJoinCost);
        if (pair.first < minJoinCost) {
            minJoinCost = pair.first;
            nodes = new vector<TreeNode*>(intermediate);
            leastCostTree = pair.second;
        } 
    } while (next_permutation(intermediate.begin(), intermediate.end()));

    this->root = nodes->front();

    printf("After creating joins. root = ");
    this->root->print();

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

    root = new WriteOutNode(root);
}

WriteOutNode::WriteOutNode(TreeNode *root) : TreeNode("Write", new Schema(root->schema)) {
    printf("Write node called \n");
    this->left = root;
    for (size_t index = 0; index < root->numberOfRelations; index++) {
        this->relNames[this->numberOfRelations++] = strdup(root->relNames[index]);
    }
}

ProjectNode::ProjectNode(TreeNode *root, NameList *attsToSelect) : TreeNode("Project", NULL)  {
    printf("ProjectNode called \n");
    this->left = root;
    for (size_t index = 0; index < root->numberOfRelations; index++) {
        this->relNames[this->numberOfRelations++] = strdup(root->relNames[index]);
    }

    Schema *rootSchema = root->schema;
    Attribute attributes[20];
    int index = 0;
    for (; attsToSelect; attsToSelect = attsToSelect->next, index++) {
        attributes[index].name = attributes->name;
        attributes[index].myType = rootSchema->FindType(attributes->name);
    }
    schema = new Schema("", index, attributes);
}

SumNode::SumNode(TreeNode *root, FuncOperator *finalFunction) : TreeNode("SumNode", constructSchemaFrom(root, finalFunction)) {
    printf("SumNode called \n");
    this->left = root;
    for (size_t index = 0; index < root->numberOfRelations; index++) {
        this->relNames[this->numberOfRelations++] = strdup(root->relNames[index]);
    }
}

Schema* SumNode::constructSchemaFrom(TreeNode *root, FuncOperator *finalFunction) {
  Schema *rootSchema = root->schema;
  Function function;
  function.GrowFromParseTree (finalFunction, *rootSchema);
  Attribute atts[2][1] = {{{"sum", Int}}, {{"sum", Double}}};
  return new Schema ("", 1, atts[function.returnsInt ? Int : Double]);
}

GroupByNode::GroupByNode(TreeNode *root, NameList *groupingAtts, FuncOperator *finalFunction) : TreeNode("GroupBy", constructSchemaFrom(root, groupingAtts, finalFunction)) {
    printf("GroupByNode called \n");
    this->left = root;
    for (size_t index = 0; index < root->numberOfRelations; index++) {
        this->relNames[this->numberOfRelations++] = strdup(root->relNames[index]);
    }
}

Schema* GroupByNode::constructSchemaFrom(TreeNode *root, NameList *groupingAtts, FuncOperator *finalFunction) {
    Function function;
    Schema *rootSchema = root->schema;
    function.GrowFromParseTree(finalFunction, *rootSchema);
    Attribute attributes[20];
    attributes[0].name = "sum"; 
    attributes[0].myType = function.returnsInt ? Int : Double;
    int numberOfAttributes = 1;
    for (; groupingAtts; groupingAtts = groupingAtts->next, numberOfAttributes++)
    {
        attributes[numberOfAttributes].name = groupingAtts->name; 
        attributes[numberOfAttributes].myType = rootSchema->FindType(groupingAtts->name);
    }
    return new Schema("", numberOfAttributes, attributes);
}

DuplicateRemovalNode::DuplicateRemovalNode(TreeNode *root) : TreeNode("DuplicateRemoval", new Schema(root->schema)) {
    printf("DuplicateRemovalNode called \n");
    this->left = root;
    for (size_t index = 0; index < root->numberOfRelations; index++) {
        this->relNames[this->numberOfRelations++] = strdup(root->relNames[index]);
    }
}

JoinNode::JoinNode(TreeNode *node1, TreeNode* node2) : TreeNode("Join", new Schema(node1->schema, node2->schema)) {
    printf("JoinNode called \n");
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

printf("Eval called with size = %d \n", order.size());
 for (std::vector<TreeNode*>::iterator runsIterator = order.begin(); runsIterator != order.end(); ++runsIterator)
	{
		TreeNode *node = *runsIterator;
		node->print();
	}

     while (order.size() > 1) {
         // Join the last two nodes
         TreeNode *node1 = order.back();
         order.pop_back(); 
         TreeNode *node2 = order.back(); 
         order.pop_back();

        JoinNode *joined = new JoinNode(node1, node2);
        order.push_back(joined);

        AndList *currentTree = buildParseTreeAfterApplyingSelect(boolean, joined->schema);
        append(builtTree, currentTree);
        
        int currentCost = this->statistics->Estimate(currentTree, joined->relNames, joined->numberOfRelations);
        // printf("currentCost = %d \n", currentCost);
        if (currentCost < 0) {
            // TODO: handle failure
            failure = true;
            break;
        }
        cost += currentCost;
        if (cost > cureentMin) {
            failure = true;
            break;
        }
     }
    //  printf("cost = %d \n", cost);
     if (failure) {
         return std::make_pair(INT_MAX, builtTree);
     }

     return std::make_pair(cost, builtTree);
}