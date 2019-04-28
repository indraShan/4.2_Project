#include "Statistics.h"
#include <iostream>
#include <vector>
#include <cstring>

using namespace std;

Relation::Relation() {
    numTuples = 0;
    joinedRelationCount = 1;
    attributes = new unordered_map<string, int>();
}

//Custom output stream for Relations
std::ostream& operator<< (std::ostream& os, const Relation& relation)
{
   os << relation.numTuples;
   os << " ";
   os << relation.joinedRelationCount;
   os << " ";
   os << relation.attributes->size();
   os << "\n";
   for (unordered_map<string, int>::iterator iit = relation.attributes->begin(); iit!=relation.attributes->end(); ++iit) { 
        os << iit->first << " " << iit->second << "\n";
   }
   return os;
}

Statistics::Statistics()
{
    store = new unordered_map<string, Relation*>();
}
//Perform deep copy of the statistics objefct
Statistics::Statistics(Statistics &copyMe)
{
    this->store = new unordered_map<string, Relation*>();
    unordered_map<string, Relation *>::iterator it;
    for (it = copyMe.store->begin(); it != copyMe.store->end(); it++)
    {
        string relName = string(it->first);
        Relation *relation = it->second;
        char cRelName[relName.size() + 1];
        strcpy(cRelName, relName.c_str());
        AddRel(cRelName, relation->numTuples);
        unordered_map<string, int>::iterator attIterator;
         for (attIterator = relation->attributes->begin(); 
            attIterator != relation->attributes->end(); attIterator++)
        {
            string attName = string(attIterator->first);
            int numDistincts = attIterator->second;
            char cAttName[attName.size() + 1];
            strcpy(cAttName, attName.c_str());
            AddAtt(cRelName, cAttName, numDistincts);
        }
        Relation *copied = this->store->find(relName)->second;
        copied->joinedRelationCount = relation->joinedRelationCount;
    }
}

Statistics::~Statistics()
{
    if (store != NULL) {
        delete store;
        store = NULL;
    }
}
//Add relations to the store map
void Statistics::AddRel(char *relName, int numTuples)
{
    Relation *relation = NULL;
	//if relation already exists, update the number of tuples, else create a new relation
    if (this->store->find(relName) == store->end()) {
        relation = new Relation();
        this->store->insert(std::make_pair(relName, relation));
    }
    else {
        relation = this->store->find(relName)->second;
    }
    relation->joinedRelationCount = 1;
    relation->numTuples = numTuples;
}
//Add attributes to the relation
void Statistics::AddAtt(char *relName, char *attName, int numDistincts)
{
    Relation *relation = NULL;
    if (this->store->find(relName) == store->end()) {
        relation = new Relation();
        this->store->insert(std::make_pair(relName, relation));
    }
    else {
        relation = this->store->find(relName)->second;
    }
	//If numDistincts is given as -1, use the number of tuples as number of distincts
	if(numDistincts == -1){
		relation->attributes->insert(std::make_pair(attName, relation->numTuples));
	}
	else{
    	relation->attributes->insert(std::make_pair(attName, numDistincts));
	}
	//Store all the attributes in a map for easy access
	attribute_relations[attName] = relName;
    //printStore();
}
void Relation::copyAttributes(Relation *rel){
	//Copy attributes from the parameter to current relation
	this->attributes->insert(rel->attributes->begin(),rel->attributes->end());
}
void Statistics::CopyRel(char *oldName, char *newName)
{
	//If relation doesn't exist
    if (this->store->find(oldName) == store->end()) {
        return;
    }
	//Create a new relation object
    Relation *relation = this->store->find(oldName)->second;
	//Add the relation to the store map with new name
    AddRel(newName, relation->numTuples);
    unordered_map<string, int>::iterator attIterator;
    for (attIterator = relation->attributes->begin(); 
    attIterator != relation->attributes->end(); attIterator++) 
    {
		//Add all the attributes to the new relation
        string attName = string(newName) + "." + attIterator->first;
        int numDistincts = attIterator->second;
        char cAttName[attName.size() + 1];
        strcpy(cAttName, attName.c_str());
        AddAtt(newName, cAttName, numDistincts);
    }
    Relation *copied = this->store->find(newName)->second;
    copied->joinedRelationCount = relation->joinedRelationCount;
}
//Read Statistics object from file
void Statistics::Read(char *fromWhere)
{
    ifstream stats_file(fromWhere);
    if (!stats_file.good()){
        return;
    }
	//Get number of relations
	int numRelations;
	std::string line;
	std::getline(stats_file, line);
	std::istringstream iss0(line);
	iss0 >> numRelations;
	for(int i = 0; i < numRelations; i++){
		//Get each relation name
		std::string relNameLine;
		Relation *relation = new Relation();
		std::getline(stats_file, relNameLine);
		std::istringstream iss(relNameLine);
		string relName;
		iss >> relName;
		std::string relationLine;		
		std::getline(stats_file, relationLine);
		std::istringstream iss2(relationLine);
		int numTuples, joinedRelations, numAtts;
		//Next line contains number of tuples, joinedRelationCount and number of attributes
		iss2 >> numTuples >> joinedRelations >> numAtts;
		relation->numTuples = numTuples;
		relation->joinedRelationCount = joinedRelations;
		for(int j = 0; j < numAtts; j++){
			//Read all the attributes line by line and add to relation object
			std::string attsLine;
			std::getline(stats_file, attsLine);
			string attr;
			int distinct;
			std::istringstream iss3(attsLine);
			iss3 >> attr >> distinct;
			relation->attributes->insert(std::make_pair(attr, distinct));
			//add the attributes to the attribute-relations map
			attribute_relations[attr] = relName;
		}
		//Insert the newly created relation into the map
		this->store->insert(std::make_pair(relName, relation));		
	}
	stats_file.close();
}

//Write statistics object to file in the following format - 
//Number of relations
//{Relation 1 Name} {Relation 1 number of tuples} {Relation 1 joined relations count}
//number of Attributes
//attribute1 numDistincts
//attribute2 numDistincts

void Statistics::Write(char *fromWhere)
{
    ofstream stats_file(fromWhere);
	stats_file << store->size() << endl;
	for (unordered_map<string, Relation*>::iterator iit = store->begin(); iit!=store->end(); ++iit){
        stats_file << iit->first << endl;
 		stats_file << iit->second->numTuples;
		stats_file << " ";
		stats_file << iit->second->joinedRelationCount;
		stats_file << " ";
		stats_file << iit->second->attributes->size();
		stats_file << "\n";
		for (unordered_map<string, int>::iterator attIterator = iit->second->attributes->begin(); attIterator!=iit->second->attributes->end(); ++attIterator) { 
				stats_file << attIterator->first << " " << attIterator->second << "\n";
		}        
    }
    stats_file.close();
}


// Review and fix.
// Are we handling all cases?
// Error handling and print.
void  Statistics::Apply(struct AndList *parseTree, char *relNames[], int numToJoin)
{
	//Check if the given relations exist in the map
    RelationsExist(relNames,numToJoin);

	// Estimate the output
	double estimate = 0.0l;
	if (0 == parseTree and numToJoin <= 2){
		double result = 1.0l;
		for (unsigned i = 0; i < numToJoin; i++){
			string relation(relNames[i]);
			result *= store->find(relation)->second->numTuples;
		}
		estimate = result;
	}
	else{
		estimate = EstimateResultFromParseTree(parseTree);
	}

	string newrelation;

	bool joinExists = false;
	AndList *aList = parseTree;
	//Check if join exists in the given parse tree
	while(aList){
		OrList *p_or = aList->left;
	 	while(p_or){
	 		ComparisonOp * comp = p_or->left;
	 		if(comp != NULL){
	 			Operand *leftop = comp->left;
	 			Operand *rightop = comp->right;
				//If both operands are names and operator equals
	 			if( (leftop != NULL && (leftop->code == NAME)) &&
		 			(rightop != NULL && (rightop->code == NAME) ) && comp->code == EQUALS) {
					joinExists = true;   
				}

		 	}
	 		p_or = p_or->rightOr;
	 	}
		aList = aList->rightAnd;

	}
	//if a join exists...
	if (joinExists){
		//Merge both the relations and update the store and merge_relations maps
		for(int i=0;i<numToJoin;i++){
			string rel(relNames[i]);
			newrelation += rel;
		}

		Relation *joinRelInfo = new Relation();
		joinRelInfo->numTuples=estimate;

		for(int i=0;i<numToJoin;i++){
			if(store->find(relNames[i]) != store->end()){
				joinRelInfo->attributes->insert(store->find(relNames[i])->second->attributes->begin(),store->find(relNames[i])->second->attributes->end());
			}
		}

		//joinRelInfo.print();
		store->insert(std::make_pair(newrelation, joinRelInfo));
		vector<string> attrs;
		//Check the parse tree to get attributes in the CNF
		while(parseTree){
			OrList *p_or = parseTree->left;
			while(p_or){
				ComparisonOp *comp = p_or->left;
				if(comp!=NULL){
					Operand *leftop = comp->left;
					if(leftop!=NULL && leftop->code == NAME){
						string attribute(leftop->value);
						
						if(attribute_relations.count(attribute)==0){
							cout << "Attribute " << attribute << " not found in given relations." << endl;
							exit(-1);
						}
						attrs.push_back(attribute);
					}
					Operand *rightop = comp->right;
					if(rightop!=NULL && rightop->code == NAME){
						string attribute(rightop->value);
						if(attribute_relations.count(attribute)==0){
							cout << "Attribute " << attribute << " not found in given relations." << endl;
							exit(-1);
						}
						attrs.push_back(attribute);
					}

				}
				p_or = p_or->rightOr;
			}
			parseTree = parseTree->rightAnd;
		}
		set<string> relationSet;
		for(auto i:attrs){
			relationSet.insert(attribute_relations[i]);
		}
		for(auto i:relationSet){
			joinRelInfo->copyAttributes(store->find(i)->second);
			store->erase(i);
		}

		for(int i=0;i<numToJoin;i++){
			store->erase(relNames[i]);
			merged_relations[relNames[i]]=newrelation;
		}

   		for (unordered_map<string, int>::iterator i = joinRelInfo->attributes->begin(); i!=joinRelInfo->attributes->end(); ++i) { 
			attribute_relations[i->first]=newrelation;
		}
	}
}
//Estimate number of tuples output
double Statistics::Estimate(struct AndList *parseTree, char **relNames, int numToJoin)
{
	//If the parse tree is empty with two relations, return cross product
	if (0 == parseTree && numToJoin <= 2){
		double result = 1.0l;
		for (signed i = 0; i < numToJoin; i++){
			string relation(relNames[i]);
			result *= store->find(relNames[i])->second->numTuples;
		}
		return result;
	}
	//Check if relations exist
	RelationsExist(relNames,numToJoin);
	//Check the parsetree and estimate output
	double result = EstimateResultFromParseTree(parseTree);
  	return result;

}
//Estimate result from parsetree
double Statistics::EstimateResultFromParseTree(struct AndList *parseTree){
	double result = 1.0l;
	bool joinExists = false;
	double selectOnlySize = 0.0l;

	while (parseTree){
	 	OrList *p_or = parseTree->left;
	 	bool independent = true;
	 	bool single = false;
	 	set <string> orset;
	 	int count = 0;

	 	// Checking for independent or dependent OR
	 	while(p_or){ 
	 		ComparisonOp * comp = p_or->left;
	 		if(comp != NULL){
	 			count++;
	 			string attribute(comp->left->value); 
	 			orset.insert(attribute);
	 		}
	 		p_or = p_or->rightOr; 
	 	}
	 	if(orset.size() != count){
	 		independent = false;
	 	}
	 	if(count == 1){
	 		independent = false;
	 		single = true;
	 	}


	 	p_or = parseTree->left;
	 	double ORresult = 0.0l;
	 	if(independent){
	 		ORresult = 1.0l;
	 	}

	 	// Estimate the output

	 	while(p_or){
	 		ComparisonOp * comp = p_or->left;
	 		if(comp != NULL){
	 			Operand *leftop = comp->left;
	 			Operand *rightop = comp->right;
	 			switch(comp->code){
					//If the operator is Equals
	 				case EQUALS:
	 					if( (leftop != NULL && (leftop->code == NAME)) &&
	 						 (rightop != NULL && (rightop->code == NAME) )) {
                        joinExists = true;
                        string lattr(leftop->value);
                        string rattr(rightop->value);
                        string lrel = attribute_relations[lattr];
                        string rrel = attribute_relations[rattr];
								
                        int lRelSize = this->store->find(lrel)->second->numTuples;
                   		
                        int lDistinct = this->store->find(lrel)->second->attributes->find(lattr)->second;
                        
                        
                        int rRelSize = this->store->find(rrel)->second->numTuples;
                        int rDistinct = this->store->find(rrel)->second->attributes->find(rattr)->second;

                        double denominator = max(lDistinct,rDistinct);
                        ORresult += (min(lRelSize,rRelSize) * (max(rRelSize,lRelSize) / denominator));
                      	
	 					}
	 					else{
	 						Operand *record = NULL;
	 						Operand *literal = NULL;
	 						if(leftop->code == NAME){
	 							record = leftop;
	 							literal = rightop;
	 						}
	 						else{
	 							record = rightop;
	 							literal = leftop;
	 						}

	 						string attribute(record->value);
							if(attribute_relations.count(attribute) == 0) break;
	 						string relation = attribute_relations[attribute];
	 						int distinct = this->store->find(relation)->second->attributes->find(attribute)->second;
	 						if(independent && !single){
	 							double prob = 1.0l - (1.0l/distinct);
		 							ORresult *= prob;
	 						}
	 						else{
	 							double prob = (1.0l/distinct);
	 								ORresult += prob;
	 						}
	 					}
	 					break;
					//If the operator is less than or greater than
	 				case LESS_THAN : case GREATER_THAN:					
					 	Operand *record = NULL;
 						Operand *literal = NULL;
 						if(leftop->code == NAME){
 							record = leftop;
 							literal = rightop;
 						}
 						else{
 							record = rightop;
 							literal = leftop;
 						}

 						string attribute(record->value);
						if(attribute_relations.count(attribute) == 0) break;

 						string relation = attribute_relations[attribute];
 
	 					if(independent){
	 						double prob = 1.0l - (1.0l/3.0l);
 							ORresult *= prob;
	 					}
	 					else{
							double prob = 1.0l/3.0l;
 							ORresult += prob;
	 					}

	 					break;
	 			}
	 			if (!joinExists){
               	Operand *record = NULL;
               	if (leftop->code == NAME)
                	{record = leftop;}
               	else if (rightop->code == NAME)
                	{record = rightop;}

					string attribute(record->value);
					if(attribute_relations.count(attribute) == 0) break;
					string relation = attribute_relations[attribute];
					int relationSize = this->store->find(relation)->second->numTuples;
					selectOnlySize = relationSize;
            }

	 		}
	 		p_or = p_or->rightOr;
	 	}

	 	if(independent){
	 		result *= (1 - ORresult);
	 	}
	 	else{
	 		result *=  ORresult;
	 	}

		parseTree = parseTree->rightAnd;

	}
	if (joinExists){ return result; }

	return result * selectOnlySize;
}

void Statistics::RelationsExist(char *relNames[],int numToJoin){
	for(int i=0;i<numToJoin;i++){
		string rel(relNames[i]);
		if(this->store->count(rel)==0 && merged_relations.count(rel)==0){
			cout << "Relation not found" << rel << endl;
			exit(-1);
		}
	}
}


int Statistics::validateParams(struct AndList *parseTree, char **relNames, int numToJoin) {
    return 1;
}

void Statistics::printStore() {
    for (unordered_map<string, Relation*>::iterator iit = store->begin(); iit!=store->end(); ++iit) { 
        std::cout << iit->first << " " << *iit->second << "\n";
    }
}

