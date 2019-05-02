
CC = g++ -O2 -Wno-deprecated

tag = -i

ifdef linux
tag = -n
endif

test.out: Record.o Comparison.o ComparisonEngine.o Schema.o File.o HeapFile.o SortedDBFile.o DBFile.o Statistics.o Pipe.o QueryMaker.o BigQ.o RelOp.o Function.o yyfunc.tab.o lex.yy.o lex.yyfunc.o test.o
	$(CC) -o test.out Record.o Comparison.o ComparisonEngine.o Schema.o File.o HeapFile.o SortedDBFile.o DBFile.o Statistics.o Pipe.o QueryMaker.o BigQ.o RelOp.o Function.o y.tab.o yyfunc.tab.o lex.yy.o lex.yyfunc.o test.o -ll -lpthread

a2-2test.out: Record.o Comparison.o ComparisonEngine.o Schema.o File.o BigQ.o HeapFile.o SortedDBFile.o DBFile.o Statistics.o Pipe.o QueryMaker.o y.tab.o Function.o lex.yy.o a2-2test.o
	$(CC) -o a2-2test.out Record.o Comparison.o ComparisonEngine.o Schema.o File.o BigQ.o HeapFile.o SortedDBFile.o DBFile.o Statistics.o Pipe.o QueryMaker.o Function.o y.tab.o lex.yy.o a2-2test.o -ll -lpthread
	
a2test.out: Record.o Comparison.o ComparisonEngine.o Schema.o File.o BigQ.o HeapFile.o SortedDBFile.o DBFile.o Statistics.o Pipe.o QueryMaker.o y.tab.o Function.o lex.yy.o a2-test.o
	$(CC) -o a2test.out Record.o Comparison.o ComparisonEngine.o Schema.o File.o BigQ.o HeapFile.o SortedDBFile.o DBFile.o Statistics.o Pipe.o QueryMaker.o Function.o y.tab.o lex.yy.o a2-test.o -ll -lpthread
	
a4-1.out: Record.o Comparison.o ComparisonEngine.o Schema.o File.o BigQ.o HeapFile.o SortedDBFile.o DBFile.o Statistics.o Pipe.o QueryMaker.o y.tab.o Function.o lex.yy.o test.o
	$(CC) -o a4-1.out Record.o Comparison.o ComparisonEngine.o Schema.o File.o BigQ.o HeapFile.o SortedDBFile.o DBFile.o Statistics.o QueryMaker.o Function.o Pipe.o y.tab.o lex.yy.o test.o -ll -lpthread
	
main:   Record.o Comparison.o ComparisonEngine.o Schema.o File.o BigQ.o HeapFile.o SortedDBFile.o DBFile.o Statistics.o Pipe.o QueryMaker.o y.tab.o Function.o lex.yy.o main.o
	$(CC) -o main Record.o Comparison.o ComparisonEngine.o Schema.o File.o BigQ.o HeapFile.o SortedDBFile.o DBFile.o Statistics.o QueryMaker.o Function.o Pipe.o y.tab.o lex.yy.o main.o -ll -lpthread

test.o: test.cc
	$(CC) -g -c test.cc
	
a2-2test.o: a2-2test.cc
	$(CC) -g -c a2-2test.cc

a2-test.o: a2-test.cc
	$(CC) -g -c a2-test.cc

a3test.o: a3test.cc
	$(CC) -g -c a3test.cc

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
		
yyfunc.tab.o: ParserFunc.y
	yacc -p "yyfunc" -b "yyfunc" -d ParserFunc.y
	#sed $(tag) yyfunc.tab.c -e "s/  __attribute__ ((__unused__))$$/# ifndef __cplusplus\n  __attribute__ ((__unused__));\n# endif/" 
	g++ -c yyfunc.tab.c
	
lex.yy.o: Lexer.l
	lex Lexer.l
	gcc  -c lex.yy.c

lex.yyfunc.o: LexerFunc.l
	lex -Pyyfunc LexerFunc.l
	gcc  -c lex.yyfunc.c

clean: 
	rm -f *.o
	rm -f *.out
	rm -f y.tab.c
	rm -f lex.yy.c
	rm -f y.tab.h


# NOTE:  *********** gtest ********************



# Makefile used from gtest github repository

# Points to the root of Google Test, relative to where this file is.
# Remember to tweak this if you move this file.
GTEST_DIR = googletest

# Points to the location of the Google Test libraries
GTEST_LIB_DIR = .

# Where to find user code.
USER_DIR = .

# Flags passed to the preprocessor.
# Set Google Test's header directory as a system directory, such that
# the compiler doesn't generate warnings in Google Test headers.
CPPFLAGS += -isystem $(GTEST_DIR)/include

# Flags passed to the C++ compiler.
CXXFLAGS += -g -Wall -Wextra -pthread -std=c++11

# Google Test libraries
GTEST_LIBS = libgtest.a libgtest_main.a

# All tests produced by this Makefile.  Remember to add new tests you
# created to the list.
TESTS = p4test


# All Google Test headers.  Usually you shouldn't change this
# definition.
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

# House-keeping build targets.

gtest : $(GTEST_LIBS) $(TESTS)

# clean :
# 	rm -f $(GTEST_LIBS) $(TESTS) *.o

# Builds gtest.a and gtest_main.a.

# Usually you shouldn't tweak such internal variables, indicated by a
# trailing _.
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS) 
# GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS) $(USER_DIR)/gtest.cc


# For simplicity and to avoid depending on Google Test's
# implementation details, the dependencies specified below are
# conservative and not optimized.  This is fine as Google Test
# compiles fast and for ordinary users its source rarely changes.
gtest-all.o : $(GTEST_SRCS_)
	$(CC) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc


libgtest.a : gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

libgtest_main.a : gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

p4test.o : $(USER_DIR)/p4test.cc \
                     $(GTEST_HEADERS)
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -g -c -v $(USER_DIR)/p4test.cc

p4test : Record.o Comparison.o ComparisonEngine.o Schema.o File.o HeapFile.o SortedDBFile.o DBFile.o Statistics.o Pipe.o QueryMaker.o BigQ.o RelOp.o Function.o y.tab.o yyfunc.tab.o lex.yy.o lex.yyfunc.o p4test.o $(GTEST_LIBS)
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -L$(GTEST_LIB_DIR) -lpthread $^ -o $@