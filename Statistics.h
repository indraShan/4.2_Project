#ifndef STATISTICS_
#define STATISTICS_
#include "ParseTree.h"
#include <unordered_map>
using namespace std;

class Relation {
public:
	int numTuples;
	int joinedRelationCount;
	unordered_map<string, int> *attributes;

	Relation();
	friend std::ostream& operator<< (std::ostream& os, const Relation& relation);
};


class Statistics
{
private:
	unordered_map<string, Relation*> *store;
	// Can we do better than storing this value here?
	double tempRes;
	void printStore();
	double Estimate(struct AndList *parseTree, char **relNames, int numToJoin, bool estimate);
	int validateParams(struct AndList *parseTree, char **relNames, int numToJoin);

public:
	Statistics();
	Statistics(Statistics &copyMe);	 // Performs deep copy
	~Statistics();


	void AddRel(char *relName, int numTuples);
	void AddAtt(char *relName, char *attName, int numDistincts);
	void CopyRel(char *oldName, char *newName);
	
	void Read(char *fromWhere);
	void Write(char *fromWhere);

	void  Apply(struct AndList *parseTree, char *relNames[], int numToJoin);
	double Estimate(struct AndList *parseTree, char **relNames, int numToJoin);

};


#endif
