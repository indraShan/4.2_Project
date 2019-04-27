#ifndef REL_OP_H
#define REL_OP_H

#include "Pipe.h"
#include "DBFile.h"
#include "Record.h"
#include "Function.h"
#include <sstream>
#include <iostream>
#include <iomanip>

class RelationalOp {
	public:
	// blocks the caller until the particular relational operator 
	// has run to completion
	virtual void WaitUntilDone () = 0;

	// tell us how much internal memory the operation can use
	virtual void Use_n_Pages (int n) = 0;
	virtual void * Run () = 0;
};

class SelectFile : public RelationalOp { 
	private:
	pthread_t thread;
	Record *buffer;
	int rlen;
	DBFile *inFile;
	Pipe *outPipe;
	CNF *selOp;
	Record *literal;
	public:
    void* Run ();
	void Run (DBFile &inFile, Pipe &outPipe, CNF &selOp, Record &literal);
	void WaitUntilDone ();
	void Use_n_Pages (int n);

};

class SelectPipe : public RelationalOp {
	private:
	pthread_t thread;
	int rlen;
	Pipe *in;
	Pipe *out;
	CNF *selop;
	Record *literal;
	public:
    void* Run ();
	void Run (Pipe &inPipe, Pipe &outPipe, CNF &selOp, Record &literal);
	void WaitUntilDone ();
	void Use_n_Pages (int n);
};
class Project : public RelationalOp { 
	private:
	pthread_t thread;
	int rlen;
	Pipe *inPipe;
	Pipe *outPipe;
	int *keepMe;
	int num_in;
	int num_out;
	public:
    void* Run ();	
	void Run (Pipe &inPipe, Pipe &outPipe, int *keepMe, int numAttsInput, int numAttsOutput);
	void WaitUntilDone ();
	void Use_n_Pages (int n);
};
class Join : public RelationalOp { 
	private:
	pthread_t thread;
	int rlen;
	Pipe *inPipeL;
	Pipe *inPipeR;
	Pipe *outPipe;
	CNF *selOp;
	Record *literal;
	public:
    void* Run ();
	void Run (Pipe &inPipeL, Pipe &inPipeR, Pipe &outPipe, CNF &selOp, Record &literal);
	void WaitUntilDone ();
	void Use_n_Pages (int n);
};
class DuplicateRemoval : public RelationalOp {
	private:
	pthread_t thread;
	int rlen;
	Pipe *inPipe;
	Pipe *outPipe;
	Schema *mySchema;
	public:
    void* Run ();
	void Run (Pipe &inPipe, Pipe &outPipe, Schema &mySchema);
	void WaitUntilDone ();
	void Use_n_Pages (int n);
};
class Sum : public RelationalOp {
	private:
	pthread_t thread;
	int rlen;
	Pipe *inPipe;
	Pipe *outPipe;
	Function *computeMe;
	public:
    void* Run ();
	void Run (Pipe &inPipe, Pipe &outPipe, Function &computeMe);
	void WaitUntilDone ();
	void Use_n_Pages (int n);
};
class GroupBy : public RelationalOp {
	private:
	pthread_t thread;
	int rlen;
	Pipe *inPipe;
	Pipe *outPipe;
	OrderMaker *groupAtts;
	Function *computeMe;
	public:
    void* Run ();
	void Run (Pipe &inPipe, Pipe &outPipe, OrderMaker &groupAtts, Function &computeMe);
	void WaitUntilDone ();
	void Use_n_Pages (int n);
};
class WriteOut : public RelationalOp {
	private:
	pthread_t thread;
	int rlen;
	Pipe *inPipe;
	FILE *outFile;
	Schema *mySchema;
	public:
    void* Run ();
	void Run (Pipe &inPipe, FILE *outFile, Schema &mySchema);
	void WaitUntilDone ();
	void Use_n_Pages (int n);
};
#endif
