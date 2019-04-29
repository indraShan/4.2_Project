
CC = g++ -O2 -Wno-deprecated

tag = -i

ifdef linux
tag = -n
endif

test.out: Record.o Comparison.o ComparisonEngine.o Schema.o File.o HeapFile.o SortedDBFile.o DBFile.o Statistics.o Pipe.o QueryMaker.o BigQ.o RelOp.o Function.o yyfunc.tab.o lex.yy.o lex.yyfunc.o test.o
	$(CC) -o test.out Record.o Comparison.o ComparisonEngine.o Schema.o File.o HeapFile.o SortedDBFile.o DBFile.o Statistics.o Pipe.o QueryMaker.o BigQ.o RelOp.o Function.o y.tab.o yyfunc.tab.o lex.yy.o lex.yyfunc.o test.o -lfl -lpthread
	
a2-2test.out: Record.o Comparison.o ComparisonEngine.o Schema.o File.o BigQ.o HeapFile.o SortedDBFile.o DBFile.o Statistics.o Pipe.o QueryMaker.o y.tab.o Function.o lex.yy.o a2-2test.o
	$(CC) -o a2-2test.out Record.o Comparison.o ComparisonEngine.o Schema.o File.o BigQ.o HeapFile.o SortedDBFile.o DBFile.o Statistics.o Pipe.o QueryMaker.o Function.o y.tab.o lex.yy.o a2-2test.o -lfl -lpthread
	
a2test.out: Record.o Comparison.o ComparisonEngine.o Schema.o File.o BigQ.o HeapFile.o SortedDBFile.o DBFile.o Statistics.o Pipe.o QueryMaker.o y.tab.o Function.o lex.yy.o a2-test.o
	$(CC) -o a2test.out Record.o Comparison.o ComparisonEngine.o Schema.o File.o BigQ.o HeapFile.o SortedDBFile.o DBFile.o Statistics.o Pipe.o QueryMaker.o Function.o y.tab.o lex.yy.o a2-test.o -lfl -lpthread
	
a4-1.out: Record.o Comparison.o ComparisonEngine.o Schema.o File.o BigQ.o HeapFile.o SortedDBFile.o DBFile.o Statistics.o Pipe.o QueryMaker.o y.tab.o Function.o lex.yy.o test.o
	$(CC) -o a4-1.out Record.o Comparison.o ComparisonEngine.o Schema.o File.o BigQ.o HeapFile.o SortedDBFile.o DBFile.o Statistics.o QueryMaker.o Function.o Pipe.o y.tab.o lex.yy.o test.o -lfl -lpthread
	
main:   Record.o Comparison.o ComparisonEngine.o Schema.o File.o BigQ.o HeapFile.o SortedDBFile.o DBFile.o Statistics.o Pipe.o QueryMaker.o y.tab.o Function.o lex.yy.o main.o
	$(CC) -o main Record.o Comparison.o ComparisonEngine.o Schema.o File.o BigQ.o HeapFile.o SortedDBFile.o DBFile.o Statistics.o QueryMaker.o Function.o Pipe.o y.tab.o lex.yy.o main.o -lfl -lpthread

test.o: test.cc
	$(CC) -g -c test.cc
	
a2-2test.o: a2-2test.cc
	$(CC) -g -c a2-2test.cc

a2-test.o: a2-test.cc
	$(CC) -g -c a2-test.cc

a1-test.o: a1-test.cc
	$(CC) -g -c a1-test.cc

QueryMaker.o: QueryMaker.cc
	$(CC) -g -c QueryMaker.cc

Statistics.o: Statistics.cc
	$(CC) -g -c Statistics.cc

Comparison.o: Comparison.cc
	$(CC) -g -c Comparison.cc
	
ComparisonEngine.o: ComparisonEngine.cc
	$(CC) -g -c ComparisonEngine.cc
	
Pipe.o: Pipe.cc
	$(CC) -g -c Pipe.cc

BigQ.o: BigQ.cc
	$(CC) -g -c BigQ.cc

HeapFile.o: HeapFile.cc
	$(CC) -g -c HeapFile.cc

SortedDBFile.o: SortedDBFile.cc
	$(CC) -g -c SortedDBFile.cc

DBFile.o: DBFile.cc
	$(CC) -g -c DBFile.cc

Function.o: Function.cc
	$(CC) -g -c Function.cc

RelOp.o: RelOp.cc
	$(CC) -g -c RelOp.cc

File.o: File.cc
	$(CC) -g -c File.cc

Record.o: Record.cc
	$(CC) -g -c Record.cc

Schema.o: Schema.cc
	$(CC) -g -c Schema.cc

main.o : main.cc
	$(CC) -g -c main.cc
	
y.tab.o: Parser.y
	yacc -d Parser.y
	#sed $(tag) y.tab.c -e "s/  __attribute__ ((__unused__))$$/# ifndef __cplusplus\n  __attribute__ ((__unused__));\n# endif/" 
	g++ -c y.tab.c

lex.yy.o: Lexer.l
	lex  Lexer.l
	gcc  -c lex.yy.c

clean: 
	rm -f *.o
	rm -f *.out
	rm -f y.tab.c
	rm -f lex.yy.c
	rm -f y.tab.h
