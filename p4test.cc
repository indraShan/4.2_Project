#include <gtest/gtest.h>
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

extern "C" struct YY_BUFFER_STATE *yy_scan_string(const char*);

void PrintOperand(struct Operand *pOperand)
{
        if(pOperand!=NULL)
        {
                cout<<pOperand->value<<" ";
        }
        else
                return;
}

void PrintComparisonOp(struct ComparisonOp *pCom)
{
        if(pCom!=NULL)
        {
                PrintOperand(pCom->left);
                switch(pCom->code)
                {
                        case 1:
                                cout<<" < "; break;
                        case 2:
                                cout<<" > "; break;
                        case 3:
                                cout<<" = ";
                }
                PrintOperand(pCom->right);

        }
        else
        {
                return;
        }
}
void PrintOrList(struct OrList *pOr)
{
        if(pOr !=NULL)
        {
                struct ComparisonOp *pCom = pOr->left;
                PrintComparisonOp(pCom);

                if(pOr->rightOr)
                {
                        cout<<" OR ";
                        PrintOrList(pOr->rightOr);
                }
        }
        else
        {
                return;
        }
}
void PrintAndList(struct AndList *pAnd)
{
        if(pAnd !=NULL)
        {
                struct OrList *pOr = pAnd->left;
                PrintOrList(pOr);
                if(pAnd->rightAnd)
                {
                        cout<<" AND ";
                        PrintAndList(pAnd->rightAnd);
                }
        }
        else
        {
                return;
        }
}

char *fileName = "Statistics.txt";
char *catalog_path = "catalog"; 

TEST(MakeQuery, Positive){

	Statistics *s = new Statistics();
        char *relName[] = { "part",  "lineitem"};

	
	s->AddRel(relName[0],200000);
	s->AddAtt(relName[0], "p_partkey",200000);
	s->AddAtt(relName[0], "p_container",40);

	s->AddRel(relName[1],6001215);
	s->AddAtt(relName[1], "l_partkey",200000);
	s->AddAtt(relName[1], "l_shipinstruct",4);
	s->AddAtt(relName[1], "l_shipmode",7);

	char *cnf = "SELECT p.part_key, p.p_container FROM part AS p WHERE (p.part_key < 1000)";

	yy_scan_string(cnf);
	yyparse();

	QueryMaker *maker = new QueryMaker(
		finalFunction,
		tables, 
		boolean, 
		groupingAtts, 
		attsToSelect, 
		distinctAtts, 
		distinctFunc, 
		s,
		catalog_path);
	maker->make();
	maker->printQuery();

	ASSERT_TRUE(maker->root != NULL);	
}

TEST(Test1, Positive) {
	
	Statistics *s = new Statistics();
	char *relName[] = {"supplier","customer","nation"};
	
	s->AddRel(relName[0],10000);
	s->AddAtt(relName[0], "s_nationkey",25);

	s->AddRel(relName[1],150000);
	s->AddAtt(relName[1], "c_custkey",150000);
	s->AddAtt(relName[1], "c_nationkey",25);
	
	s->AddRel(relName[2],25);
	s->AddAtt(relName[2], "n_nationkey",25);

		
	char *cnf = "SELECT SUM(s.s_nationkey) FROM supplier AS s, nation AS n WHERE (s.s_nationkey = n.n_nationkey) AND (s.s_nationkey < 20) GROUP BY s.s_nationkey";

	yy_scan_string(cnf);
	yyparse();

	QueryMaker *maker = new QueryMaker(
		finalFunction,
		tables, 
		boolean, 
		groupingAtts, 
		attsToSelect, 
		distinctAtts, 
		distinctFunc, 
		s,
		catalog_path);
	maker->make();
	maker->printQuery();

	ASSERT_TRUE(maker->root != NULL);

}

TEST(Test2, Positive){

	Statistics *s = new Statistics();
    char *relName[] = {"orders","customer","nation"};


	s->AddRel(relName[0],1500000);
	s->AddAtt(relName[0], "o_custkey",150000);

	s->AddRel(relName[1],150000);
	s->AddAtt(relName[1], "c_custkey",150000);
	s->AddAtt(relName[1], "c_nationkey",25);
	
	s->AddRel(relName[2],25);
	s->AddAtt(relName[2], "n_nationkey",25);

	char *cnf = "SELECT SUM(c.c_custkey), n.n_nationkey FROM customer AS c, nation AS n WHERE (c.c_nationkey = n.n_nationkey)";
	yy_scan_string(cnf);
	yyparse();

	QueryMaker *maker = new QueryMaker(
		finalFunction,
		tables, 
		boolean, 
		groupingAtts, 
		attsToSelect, 
		distinctAtts, 
		distinctFunc, 
		s,
		catalog_path);
	maker->make();
	maker->printQuery();

	ASSERT_TRUE(maker->root != NULL);

	
	
}

TEST(Test3, Positive){

	Statistics *s = new Statistics();
	char *relName[] = {"supplier","customer","nation"};

	
	s->AddRel(relName[0],10000);
	s->AddAtt(relName[0], "s_nationkey",25);

	s->AddRel(relName[1],150000);
	s->AddAtt(relName[1], "c_custkey",150000);
	s->AddAtt(relName[1], "c_nationkey",25);
	
	s->AddRel(relName[2],25);
	s->AddAtt(relName[2], "n_nationkey",25);


	// s.printStore();
	char *cnf = "SELECT DISTINCT s_nationkey FROM supplier AS s, nation AS n WHERE (s.s_nationkey = n.n_nationkey)";
	yy_scan_string(cnf);
	yyparse();	

	QueryMaker *maker = new QueryMaker(
		finalFunction,
		tables, 
		boolean, 
		groupingAtts, 
		attsToSelect, 
		distinctAtts, 
		distinctFunc, 
		s,
		catalog_path);
	maker->make();
	maker->printQuery();

	ASSERT_TRUE(maker->root != NULL);

}



TEST(Test4, Positive){

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


	char *cnf = "SELECT DISTINCT ps.ps_suppkey, s.s_nationkey FROM partsupp AS ps, supplier AS s WHERE (s_suppkey = ps_suppkey)";
	yy_scan_string(cnf);
	yyparse();

	QueryMaker *maker = new QueryMaker(
		finalFunction,
		tables, 
		boolean, 
		groupingAtts, 
		attsToSelect, 
		distinctAtts, 
		distinctFunc, 
		s,
		catalog_path);
	maker->make();
	maker->printQuery();

	ASSERT_TRUE(maker->root != NULL);
	
	

}

TEST(Test5, Positive){

	Statistics *s = new Statistics();
        char *relName[] = { "part",  "partsupp"};

	
	s->AddRel(relName[0],200000);
	s->AddAtt(relName[0], "p_partkey",200000);
	s->AddAtt(relName[0], "p_size",50);

	s->AddRel(relName[1],800000);
	s->AddAtt(relName[1], "ps_partkey",200000);
	

	char *cnf = "SELECT p.p_size FROM part AS p, partsupp AS ps WHERE (p.p_partkey=ps.ps_partkey) AND (p.p_size =3 OR p.p_size=6 OR p.p_size =19)";

	yy_scan_string(cnf);
	yyparse();
	

	QueryMaker *maker = new QueryMaker(
		finalFunction,
		tables, 
		boolean, 
		groupingAtts, 
		attsToSelect, 
		distinctAtts, 
		distinctFunc, 
		s,
		catalog_path);
	maker->make();
	maker->printQuery();

	ASSERT_TRUE(maker->root != NULL);

}
TEST(Test6, Positive){

	Statistics *s = new Statistics();
        char *relName[] = { "part",  "partsupp","supplier"};

	
	s->AddRel(relName[0],200000);
	s->AddAtt(relName[0], "p_partkey",200000);
	s->AddAtt(relName[0], "p_name", 199996);

	s->AddRel(relName[1],800000);
	s->AddAtt(relName[1], "ps_partkey",200000);
	s->AddAtt(relName[1], "ps_suppkey",10000);
	
	s->AddRel(relName[2],10000);
	s->AddAtt(relName[2], "s_suppkey",10000);
	
	char *cnf = "SELECT p_name, ps_suppkey FROM part AS p, partsupp AS ps WHERE (p.p_partkey=ps.ps_partkey) AND (p.p_name = 'dark green antique puff wheat') ";
	yy_scan_string(cnf);
	yyparse();

	QueryMaker *maker = new QueryMaker(
		finalFunction,
		tables, 
		boolean, 
		groupingAtts, 
		attsToSelect, 
		distinctAtts, 
		distinctFunc, 
		s,
		catalog_path);
	maker->make();
	maker->printQuery();

	ASSERT_TRUE(maker->root != NULL);
	
	

}

TEST(Test7, Positive){

	Statistics *s = new Statistics();
        char *relName[] = { "part",  "lineitem"};

	
	s->AddRel(relName[0],200000);
	s->AddAtt(relName[0], "p_partkey",200000);
	s->AddAtt(relName[0], "p_container",40);

	s->AddRel(relName[1],6001215);
	s->AddAtt(relName[1], "l_partkey",200000);
	s->AddAtt(relName[1], "l_shipinstruct",4);
	s->AddAtt(relName[1], "l_shipmode",7);

	char *cnf = "SELECT p.p_container, l.l_shipmode FROM part AS p, lineitem AS l WHERE (l.l_partkey = p.p_partkey) AND (l.l_shipmode = 'AIR' OR l.l_shipmode = 'AIR REG') AND (p.p_container ='SM BOX' OR p.p_container = 'SM PACK')  AND (l.l_shipinstruct = 'DELIVER IN PERSON')";

	yy_scan_string(cnf);
	yyparse();
	

	QueryMaker *maker = new QueryMaker(
		finalFunction,
		tables, 
		boolean, 
		groupingAtts, 
		attsToSelect, 
		distinctAtts, 
		distinctFunc, 
		s,
		catalog_path);
	maker->make();
	maker->printQuery();

	ASSERT_TRUE(maker->root != NULL);
	
	
}

int main (int argc, char *argv[]) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
