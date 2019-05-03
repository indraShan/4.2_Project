
#include <iostream>
#include "ParseTree.h"
#include "Statistics.h"
#include "QueryMaker.h"
#include "DBFile.h"

using namespace std;

extern "C" {
	int yyparse(void);   // defined in y.tab.c
}

extern "C" struct YY_BUFFER_STATE *yy_scan_string(const char*);

extern struct FuncOperator *finalFunction; // the aggregate function (NULL if no agg)
extern struct TableList *tables; // the list of tables and aliases in the query
extern struct AndList *boolean; // the predicate in the WHERE clause
extern struct NameList *groupingAtts; // grouping atts (NULL if no grouping)
extern struct NameList *attsToSelect; // the set of attributes in the SELECT (NULL if no such atts)
extern int distinctAtts; // 1 if there is a DISTINCT in a non-aggregate query 
extern int distinctFunc;  // 1 if there is a DISTINCT in an aggregate query
char *catalog_path = "catalog";
int main () {
	
	
	// const char *tpch_dir ="/Users/indrajit/Documents/Study/UFL/Spring19/DBI/Project/1/data/";
	// char *supplier = "supplier"; 
	// char *nation = "supplier"; 

	// DBFile dbfile;
	// int result = dbfile.Create ("supplier.bin", heap, NULL);
	// Schema schema(catalog_path, supplier);
	// char tbl_path[100];
	// sprintf (tbl_path, "%s%s.tbl", tpch_dir, "supplier"); 
	// dbfile.Load(schema, tbl_path);
	// result = dbfile.Close();

	// DBFile dbfile2;
	// result = dbfile2.Create ("nation.bin", heap, NULL);
	// Schema schema2(catalog_path, nation);
	// char tbl_path2[100];
	// sprintf (tbl_path2, "%s%s.tbl", tpch_dir, "nation"); 
	// dbfile2.Load(schema2, tbl_path2);
	// result = dbfile2.Close();

	Statistics *s = new Statistics();
        char *relName[] = { "partsupp", "supplier", "nation"};

	
	s->AddRel(relName[0],800000);
	s->AddAtt(relName[0], "ps_suppkey",10000);

	s->AddRel(relName[1],10000);
	s->AddAtt(relName[1], "s_suppkey",10000);
	s->AddAtt(relName[1], "s_nationkey",25);
	
	s->AddRel(relName[2],25);
	s->AddAtt(relName[2], "n_nationkey",25);
	s->AddAtt(relName[2], "n_name",25);


	char *cnf = "SELECT	s.s_suppkey, ps.ps_suppkey FROM partsupp AS ps, supplier AS s WHERE (s.s_suppkey = ps.ps_suppkey)";
	yy_scan_string(cnf);
	yyparse();	

	    QueryMaker *maker = new QueryMaker(s);
	maker->plan();
	
	maker->print();
	// maker->runQuery();
	// if (maker->root != NULL) {
	// 	maker->root->run();
	// }
	
	
	// s.AddRel(relName[0], 10000);
	// s.AddAtt(relName[0], "o_orderkey", 10000);
	// s.AddAtt(relName[0], "o_totalprice", 10000);
	// s.AddAtt(relName[0], "o_custkey", 10000);

	// s.AddRel(relName[1],800000);
	// s.AddAtt(relName[1], "c_custkey", 10000);	

	// Statistics *statistics = new Statistics();
	// statistics->Read("Statistics.txt");
	// statistics->printStore();

	// delete statistics;
	// statistics = NULL;
}


