#include "RelOp.h"


//Generic worker thread started by each relation
void* RelWorkerThread (void* args)
{
	RelationalOp *relationalOp = (RelationalOp *) args;
	relationalOp->Run ();
}

// SelectPipe takes two pipes as input: an input pipe and an output pipe. 
// It also takes a CNF. It simply applies that CNF to every tuple that comes through the pipe, 
// and every tuple that is accepted is stuffed into the output pipe
void * SelectPipe::Run () {
	ComparisonEngine ceng;
	Record rec;
	while(in->Remove(&rec)){
		if(ceng.Compare(&rec,literal,selop)==1){
			out->Insert(&rec);
		}
	}
	out->ShutDown();
}

void SelectPipe::Run (Pipe &inPipe, Pipe &outPipe, CNF &selOp, Record &literal) {
	this->in = &inPipe;
	this->out = &outPipe;
	this->selop = &selOp;
	this->literal = &literal;
  	pthread_create (&thread, NULL, RelWorkerThread ,this);
}

void SelectPipe::WaitUntilDone () {
	pthread_join (thread, NULL);
}

void SelectPipe::Use_n_Pages (int runlen) {
	rlen = runlen;
}

// SelectFile takes a DBFile and a pipe as input. 
// You can assume that this file is all set up; it has been opened and is ready to go. 
// It also takes a CNF. It then performs a scan of the underlying file, 
// and for every tuple accepted by the CNF, it stuffs the tuple into the pipe as output. 
// The DBFile should not be closed by the SelectFile class; that is the job of the caller.
void * SelectFile::Run () {
	
	ComparisonEngine ceng;
	Record rec;
	while(inFile->GetNext(rec, *selOp, *literal)){
			outPipe->Insert(&rec);
	}
	outPipe->ShutDown();
}


void SelectFile::Run (DBFile &inFile, Pipe &outPipe, CNF &selOp, Record &literal) {
	this->inFile = &inFile;
	this->outPipe = &outPipe;
	this->selOp = &selOp;
	this->literal = &literal;
  	pthread_create (&thread, NULL, RelWorkerThread ,this); 
}

void SelectFile::WaitUntilDone () {
	pthread_join (thread, NULL);
}

void SelectFile::Use_n_Pages (int runlen) {
	rlen = runlen;
}
// Project takes an input pipe and an output pipe as input. It also takes an array of integers keepMe as well as the
// number of attributes for the records coming through the input pipe and the number of attributes to keep from those
// input records. The array of integers tells Project which attributes to keep from the input records, and which order
// to put them in. So, for example, say that the array keepMe had the values [3, 5, 7, 1]. This means that Project 
// should take the third attribute from every input record and treat it as the first attribute of those records that 
// it puts into the output pipe. Project should take the fifth attribute from every input record and treat it as 
// the second attribute of every record that it puts into the output pipe. The seventh input attribute becomes the 
// third. And so on.
void * Project::Run () {
	for(int i = 0;i< num_out;i++){
		cout << keepMe[i] << endl;
	}
	while(true){
	Record * temp = new Record;
		if(inPipe->Remove(temp)){
			temp->Project(keepMe,num_out,num_in);
			outPipe->Insert(temp);
		}	
		else{
			break;
		}	
	}
	outPipe->ShutDown();
}
void Project::Run (Pipe &inPipe, Pipe &outPipe, int *keepMe, int numAttsInput, int numAttsOutput) {
	this->inPipe = &inPipe;
	this->outPipe = &outPipe;
	this->keepMe = keepMe;
	this->num_in = numAttsInput;
	this->num_out = numAttsOutput;
  	pthread_create (&thread, NULL, RelWorkerThread ,this);
}

void Project::WaitUntilDone () {
	pthread_join (thread, NULL);
}

void Project::Use_n_Pages (int runlen) {
	rlen = runlen;
}
// Sum computes the SUM SQL aggregate function over the input pipe, and puts a single tuple into the output pipe that has the sum.
void * Sum::Run () {
	Record rec;
	double sum = 0.0;
	while (inPipe->Remove (&rec)){
		int ival = 0; 
		double dval = 0.0;
		computeMe->Apply(rec, ival, dval);
		sum += (ival + dval);
	}
	Attribute DA = {"double", Double};
	Schema out_sch ("out_sch", 1, &DA);
	char buffer[32];
  	sprintf(buffer, "%1.2f", sum);

	ostringstream outstream;
    outstream << buffer;
    string str = outstream.str() + "|";
	rec.ComposeRecord(&out_sch,str.c_str());
	rec.Print(&out_sch);
	outPipe->Insert(&rec);
	outPipe->ShutDown();
}
void Sum::Run (Pipe &inPipe, Pipe &outPipe, Function &computeMe) {
	this->inPipe = &inPipe;
	this->outPipe = &outPipe;
	this->computeMe = &computeMe;
  	pthread_create (&thread, NULL, RelWorkerThread ,this);
}
void Sum::WaitUntilDone () {
	pthread_join (thread, NULL);
}

void Sum::Use_n_Pages (int runlen) {
	rlen = runlen;
}
// DuplicateRemoval takes an input pipe, an output pipe, as well as the schema for the tuples coming through the input pipe, 
// and does a duplicate removal. That is, everything that somes through the output pipe will be distinct. 
//It will use the BigQ class to do the duplicate removal. The OrderMaker that will be used by the BigQ 
//(which you’ll need to write some code to create) will simply list all of the attributes from the input tuples.
void * DuplicateRemoval::Run () {
	OrderMaker * ord  = new OrderMaker(mySchema);
	DBFile dbfile;
	struct {OrderMaker *o; int l;} startup = {ord, rlen};
	char * path = "./temp.bin";
	dbfile.Create (path, sorted, &startup);
	dbfile.Close();
	dbfile.Open(path);
	dbfile.MoveFirst ();
	Record temp;
	while(inPipe->Remove(&temp)){
		dbfile.Add(temp);
	}
	dbfile.MoveFirst ();
	Record recs[2];
	ComparisonEngine comp;
	Record *lasts = NULL, *prevs = NULL;
	int j = 0;
	while (dbfile.GetNext(recs[j%2])) {
		prevs = lasts;
		lasts = &recs[j%2];
		if (prevs && lasts) {
			if (comp.Compare (prevs, lasts, ord) == 0) {
			}
			else{
				outPipe->Insert(prevs);
			}
		}
		j++;
	}
	outPipe->Insert(lasts);
	outPipe->ShutDown();
}
void DuplicateRemoval::Run (Pipe &inPipe, Pipe &outPipe, Schema &mySchema) {
	this->inPipe = &inPipe;
	this->outPipe = &outPipe;
	this->mySchema = &mySchema;
  	pthread_create (&thread, NULL, RelWorkerThread ,this);
}

void DuplicateRemoval::WaitUntilDone () {
	pthread_join (thread, NULL);
}

void DuplicateRemoval::Use_n_Pages (int runlen) {
	rlen = runlen;
}

// WriteOut accepts an input pipe, a schema, and a FILE*, and uses the schema to write text version of the output records to the file.
void * WriteOut::Run () {
	Record temp;
	while(inPipe->Remove(&temp)){
		int n = mySchema->GetNumAtts();
		Attribute *atts = mySchema->GetAtts();
		// loop through all of the attributes
		for (int i = 0; i < n; i++) {
			// print the attribute name
			fprintf(outFile, "%s: ", atts[i].name);
			// use the i^th slot at the head of the record to get the
			// offset to the correct attribute in the record
			int pointer = ((int *) temp.bits)[i + 1];

			// here we determine the type, which given in the schema;
			// depending on the type we then print out the contents
			fprintf(outFile, "[");
			// first is integer
			if (atts[i].myType == Int) {
				int *myInt = (int *) &(temp.bits[pointer]);
				fprintf(outFile, "%d", *myInt);
			// then is a double
			} else if (atts[i].myType == Double) {
				double *myDouble = (double *) &(temp.bits[pointer]);
				fprintf(outFile, "%f", *myDouble);	
			// then is a character string
			} else if (atts[i].myType == String) {
				char *myString = (char *) &(temp.bits[pointer]);
				fprintf(outFile, "%s", myString);	
			} 

			fprintf(outFile, "]");
			// print out a comma as needed to make things pretty
			if (i != n - 1) {
				fprintf(outFile, ",");
			}
		}
		fprintf(outFile, "\n");
	}
}
void WriteOut::Run (Pipe &inPipe, FILE *outFile, Schema &mySchema) {
	this->inPipe = &inPipe;
	this->outFile = outFile;
	this->mySchema = &mySchema;
  	pthread_create (&thread, NULL, RelWorkerThread ,this);
}

void WriteOut::WaitUntilDone () {
	pthread_join (thread, NULL);
}

void WriteOut::Use_n_Pages (int runlen) {
	rlen = runlen;
}

// GroupBy is a lot like Sum, except that it does grouping, and then puts one sum into the output pipe for each group.
// Every tuple put into the output pipe has a sum as the first attribute, followed by the values for each 
// of the grouping attributes as the remainder of the attributes. The grouping is specified using an instance 
// of the OrderMaker class that is passed in. The sum to compute is given in an instance of the Function class.

void * GroupBy::Run () {
	DBFile dbfile;
	struct {OrderMaker *o; int l;} startup = {groupAtts, rlen};
	char * path = "./group_temp.bin";
	dbfile.Create (path, sorted, &startup);
	dbfile.Close();
	dbfile.Open(path);
	dbfile.MoveFirst ();
	Record temp;
	while(inPipe->Remove(&temp)){
		dbfile.Add(temp);
	}
	dbfile.MoveFirst ();
	Record recs[2];
	ComparisonEngine comp;
	Record *lasts = NULL, *prevs = NULL;
	int j = 0;
	double sum = 0.0;
	Attribute DA = {"double", Double};
	Schema out_sch ("out_sch", 1, &DA);
	while (dbfile.GetNext(recs[j%2])) {
		prevs = lasts;
		lasts = &recs[j%2];
		if (prevs && lasts) {
			int ival = 0; 
			double dval = 0.0;
			computeMe->Apply(*prevs, ival, dval);
			sum += (ival + dval);
			if (comp.Compare (prevs, lasts, groupAtts) == 0) {
				
			}
			else{
				// diff records - generate new record for prevs group
				char buffer[32];
  				sprintf(buffer, "%1.2f", sum);
				ostringstream outstream;
    			outstream << buffer;
    			string str = outstream.str() + "|";
				prevs->ComposeRecord(&out_sch,str.c_str());
				prevs->Print(&out_sch);
				sum = 0.0;
				outPipe->Insert(prevs);
			}
		}
		j++;
	}
	int ival = 0; 
	double dval = 0.0;
	computeMe->Apply(*lasts, ival, dval);
	sum += (ival + dval);
	char buffer[32];
	sprintf(buffer, "%1.2f", sum);
	ostringstream outstream;
	outstream << buffer;
	string str = outstream.str() + "|";
	lasts->ComposeRecord(&out_sch,str.c_str());
	outstream.clear();
	outPipe->Insert(lasts);
	dbfile.Close();
	outPipe->ShutDown();

}
void GroupBy::Run (Pipe &inPipe, Pipe &outPipe, OrderMaker &groupAtts, Function &computeMe) {
	this->inPipe = &inPipe;
	this->outPipe = &outPipe;
	this->groupAtts = &groupAtts;
	this->computeMe = &computeMe;
  	pthread_create (&thread, NULL, RelWorkerThread ,this);
}

void GroupBy::WaitUntilDone () {
	pthread_join (thread, NULL);
}

void GroupBy::Use_n_Pages (int runlen) {
	rlen = runlen;
}

// Join takes two input pipes, an output pipe, and a CNF, and joins all of the records from the two pipes according to that CNF.
// Join should use a BigQ to store all of the tuples coming from the left input pipe, and a second BigQ for the right input pipe, 
// and then perform a merge in order to join the two input pipes. You’ll create the OrderMakers for the two BigQ’s using 
// the CNF (the function GetSortOrders will be used to create the OrderMakers). If you can’t get an appropriate pair of 
// OrderMakers because the CNF can’t be implemented using a sort-merge join (due to the fact it does not have an equality check)
// then your Join operation should default to a block-nested loops join.
void* Join::Run () {
	OrderMaker so1,so2;
	selOp->GetSortOrders(so1,so2);
	DBFile dbLeft,dbRight;
	struct {OrderMaker *o; int l;} startup1 = {&so1, rlen};
	struct {OrderMaker *o; int l;} startup2 = {&so2, rlen};

	char *p1 = "./join_left_temp.bin";
	char *p2 = "./join_right_temp.bin";
	remove(p1);
	remove(p2);
	dbLeft.Create (p1, sorted, &startup1);
	dbRight.Create (p2, sorted, &startup2);
	dbLeft.Close();
	dbLeft.Open(p1);
	dbRight.Close();
	dbRight.Open(p2);
	Record temp;
	int leftAtts=0, rightAtts=0;
	while(inPipeL->Remove(&temp)){
		leftAtts = temp.GetNumAtts();
		dbLeft.Add(temp);
	}
	while(inPipeR->Remove(&temp)){
		rightAtts = temp.GetNumAtts();
		dbRight.Add(temp);
	}
	int A[leftAtts+rightAtts];
	int i = 0;
	for(i = 0;i<leftAtts;i++){
		A[i] = i;
	}
	for(int k=0;k<rightAtts;k++){
		A[i]=k;
		i++;
	}
	dbLeft.MoveFirst();
	
	dbRight.MoveFirst();
	ComparisonEngine comp;
	Record leftRecord,rightRecord;
	// cout << "starting comparison merger" << endl;
	int leftFlag = 0;
	int rightFlag = 0;
	if(dbLeft.GetNext(leftRecord) && dbRight.GetNext(rightRecord)){
		leftFlag = 1;
		rightFlag= 1;
	}
	while(true){
		if(leftFlag && rightFlag){
			int res = comp.Compare(&leftRecord,&so1,&rightRecord,&so2);
			if(res == 0){
				// merge
				Record *temp = new Record();
				temp->MergeRecords (&leftRecord,&rightRecord,leftAtts,rightAtts,A,leftAtts+rightAtts-(so2.numAtts),leftAtts);		
				// push to output
				outPipe->Insert(temp);
				if(!(dbLeft.GetNext(leftRecord) && dbRight.GetNext(rightRecord))){
					leftFlag = 0;
					rightFlag= 0;
				}
			}
			else if(res == -1){
				if(!dbLeft.GetNext(leftRecord)) {leftFlag = 0; break;}
			}
			else{
				if(!dbRight.GetNext(rightRecord)) {rightFlag = 0; break;}
			}
		}
		else{
			break;
		}
	}
	dbLeft.Close();
	dbRight.Close();
	outPipe->ShutDown();
}

void Join::Run (Pipe &inPipeL, Pipe &inPipeR, Pipe &outPipe, CNF &selOp, Record &literal) {
	this->inPipeL = &inPipeL;
	this->inPipeR = &inPipeR;
	this->outPipe = &outPipe;
	this->literal = &literal;
	this->selOp = &selOp;
  	pthread_create (&thread, NULL, RelWorkerThread ,this);
}

void Join::WaitUntilDone () {
	pthread_join (thread, NULL);
}

void Join::Use_n_Pages (int runlen) {
	rlen = runlen;
}