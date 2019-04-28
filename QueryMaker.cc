#include <iostream>
#include <stdlib.h> 
#include "QueryMaker.h"
#include <algorithm>
#include <climits>
#include <stdlib.h>
#include <string.h>


TreeNode::TreeNode(char *fpath, char *relName) {
    if (fpath != NULL) {
        this->schema = new Schema(fpath, relName);
    }
    this->estimatedCost = 0;
    if (relName != NULL) {
        this->relNames[this->numberOfRelations] = relName;
        this->numberOfRelations = 1;
    }
}

TreeNode::~TreeNode() {

}

void TreeNode::print() {
    if (this->left != NULL) {
        this->left->print();
    }
    // Print this node's data here.
    printSelf();

    if (this->right != NULL) {
        this->right->print();
    }
}

void TreeNode::printSelf() {
    printf("Called in base class \n");
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

FileNode::FileNode(TableList *table, char *fpath, char *relName, char *alias) : TreeNode(fpath, relName) {
    this->table = table;
    this->schema->setAlias(alias);
}

void FileNode::printSelf() {
    printf("Table name =%s, aliasAs = %s \n", table->tableName, table->aliasAs);
}

void QueryMaker::make() {
    // Make tables.
    AndList *leastCostTree = NULL;
    for (TableList* table = tables; table; table = table->next) {
        this->statistics->CopyRel(table->tableName, table->aliasAs);
        FileNode *node = new FileNode(table, this->catolog_path, table->tableName, table->aliasAs);
        this->nodes->push_back(node);
    }
    // // If there are multiple tables, find the most efficient join order.
    int minJoinCost = INT_MAX;
    
    vector<TreeNode *> intermediate(*nodes);
    
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

    // Get the root.
    this->root = nodes->front();
    printf("Final cost = %d \n", minJoinCost);

    if (groupingAtts) {
        if (distinctFunc) {
            root = new DuplicateRemovalNode(root);
        }
    root = new GroupByNode(groupingAtts, finalFunction, root);
  }
 else if (finalFunction) {
//     root = new SumNode(finalFunction, root);
//   }

    // 
}

DuplicateRemovalNode::DuplicateRemovalNode(TreeNode *root) : TreeNode(NULL, NULL) {
    this->schema = new Schema(root->schema);
    this->left = root;
    for (size_t index = 0; index < root->numberOfRelations; index++) {
        this->relNames[this->numberOfRelations++] = strdup(root->relNames[index]);
    }
}

JoinNode::JoinNode(TreeNode *node1, TreeNode* node2) : TreeNode(NULL, NULL) {
    this->schema = new Schema(node1->schema, node2->schema);
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
     printf("cost = %d \n", cost);
     if (failure) {
         return std::make_pair(INT_MAX, builtTree);
     }

     return std::make_pair(cost, builtTree);
}