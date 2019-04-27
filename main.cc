
#include <iostream>
#include "ParseTree.h"
#include "Statistics.h"

using namespace std;

extern "C" {
	int yyparse(void);   // defined in y.tab.c
}

int main () {
	Statistics *statistics = new Statistics();
	statistics->Read("");

	yyparse();
	QueryMaker *maker = new QueryMaker();
	//  
	// maker.make()
	// maker.printQuery()

	delete statistics;
	statistics = NULL;
}


