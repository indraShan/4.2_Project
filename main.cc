
#include <iostream>
#include "ParseTree.h"
#include "Statistics.h"
#include "QueryMaker.h"

using namespace std;

extern "C" {
	int yyparse(void);   // defined in y.tab.c
}

extern struct FuncOperator *finalFunction; // the aggregate function (NULL if no agg)
extern struct TableList *tables; // the list of tables and aliases in the query
extern struct AndList *boolean; // the predicate in the WHERE clause
extern struct NameList *groupingAtts; // grouping atts (NULL if no grouping)
extern struct NameList *attsToSelect; // the set of attributes in the SELECT (NULL if no such atts)
extern int distinctAtts; // 1 if there is a DISTINCT in a non-aggregate query 
extern int distinctFunc;  // 1 if there is a DISTINCT in an aggregate query

int main () {

	// Statistics s;
    // char *relName[] = {"orders", "customer"};

	
	// s.AddRel(relName[0], 10000);
	// s.AddAtt(relName[0], "o_orderkey", 10000);
	// s.AddAtt(relName[0], "o_totalprice", 10000);
	// s.AddAtt(relName[0], "o_custkey", 10000);

	// s.AddRel(relName[1],800000);
	// s.AddAtt(relName[1], "c_custkey", 10000);	
	// s.Write("new_stats.txt");


	char *catalog_path = "catalog"; 
	Statistics *statistics = new Statistics();
	statistics->Read("Statistics.txt");
	statistics->printStore();
	yyparse();
	QueryMaker *maker = new QueryMaker(
		finalFunction,
		tables, 
		boolean, 
		groupingAtts, 
		attsToSelect, 
		distinctAtts, 
		distinctFunc, 
		statistics,
		catalog_path);
	maker->make();
	// maker->printQuery();

	delete statistics;
	statistics = NULL;
}


