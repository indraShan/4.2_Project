#ifndef STATISTICS_
#define STATISTICS_
#include "ParseTree.h"
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <set>
using namespace std;

class Relation {
public:
	int numTuples;
	int joinedRelationCount;
	unordered_map<string, int> *attributes;
	Relation();
	friend std::ostream& operator<< (std::ostream& os, const Relation& relation);
	friend std::istream& operator>>(std::istream& is, Relation& relation);

	void copyAttributes(Relation *rel);
};


class Statistics
{
private:
	unordered_map<string, Relation*> *store;
	unordered_map<string,string> attribute_relations;
	unordered_map<string,string> merged_relations;
	// Can we do better than storing this value here?
	double tempRes;
	double Estimate(struct AndList *parseTree, char **relNames, int numToJoin, bool estimate);
	int validateParams(struct AndList *parseTree, char **relNames, int numToJoin);

public:
	Statistics();
	Statistics(Statistics &copyMe);	 // Performs deep copy
	~Statistics();

	void printStore();

	void AddRel(char *relName, int numTuples);
	void AddAtt(char *relName, char *attName, int numDistincts);
	void CopyRel(char *oldName, char *newName);
	
	void Read(char *fromWhere);
	void Write(char *fromWhere);

	void  Apply(struct AndList *parseTree, char *relNames[], int numToJoin);
	double Estimate(struct AndList *parseTree, char **relNames, int numToJoin);
	double EstimateResultFromParseTree(struct AndList *p_And);
	void RelationsExist(char *relNames[],int numToJoin);

};


#endif
