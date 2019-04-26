#include "Statistics.h"
#include <iostream>
#include <vector>
#include <cstring>

Relation::Relation() {
    numTuples = 0;
    joinedRelationCount = 1;
    attributes = new unordered_map<string, int>();
}


std::ostream& operator<< (std::ostream& os, const Relation& relation)
{
   os << relation.numTuples;
   os << " ";
   for (unordered_map<string, int>::iterator iit = relation.attributes->begin(); iit!=relation.attributes->end(); ++iit) { 
        os << iit->first << " " << iit->second << "\n";
   }
   return os;
}

Statistics::Statistics()
{
    store = new unordered_map<string, Relation*>();
}

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

void Statistics::AddRel(char *relName, int numTuples)
{
    Relation *relation = NULL;
    if (this->store->find(relName) == store->end()) {
        relation = new Relation();
        this->store->insert(std::make_pair(relName, relation));
    }
    else {
        relation = this->store->find(relName)->second;
    }
    // TODO: Update or increment?
    relation->joinedRelationCount = 1;
    relation->numTuples = numTuples;
}

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
    // TODO: Update or increment?
    relation->attributes->insert(std::make_pair(attName, numDistincts));
    printStore();
}

void Statistics::CopyRel(char *oldName, char *newName)
{
    if (this->store->find(oldName) == store->end()) {
        return;
    }
    Relation *relation = this->store->find(oldName)->second;
    AddRel(newName, relation->numTuples);
    unordered_map<string, int>::iterator attIterator;
    for (attIterator = relation->attributes->begin(); 
    attIterator != relation->attributes->end(); attIterator++) 
    {
        string attName = attIterator->first;
        int numDistincts = attIterator->second;
        char cAttName[attName.size() + 1];
        strcpy(cAttName, attName.c_str());
        AddAtt(newName, cAttName, numDistincts);
    }
    Relation *copied = this->store->find(newName)->second;
    copied->joinedRelationCount = relation->joinedRelationCount;
}
	
void Statistics::Read(char *fromWhere)
{
}
void Statistics::Write(char *fromWhere)
{
}

// Review and fix.
// Are we handling all cases?
// Error handling and print.
void  Statistics::Apply(struct AndList *parseTree, char *relNames[], int numToJoin)
{
    struct AndList * andlist = parseTree;
	struct OrList * orlist;
	while (andlist != NULL){
		if (andlist->left != NULL){
			orlist = andlist->left;
			while(orlist != NULL){
				if (orlist->left->left->code == 3 && orlist->left->right->code == 3){//
					unordered_map<string, int>::iterator itAtt[2];
					unordered_map<string, Relation*>::iterator itRel[2];
					string joinAtt1(orlist->left->left->value);
					string joinAtt2(orlist->left->right->value);
					for (unordered_map<string, Relation*>::iterator iit = store->begin(); iit!=store->end(); ++iit){
						itAtt[0] = iit->second->attributes->find(joinAtt1);
						if(itAtt[0] != iit->second->attributes->end()){
							itRel[0] = iit;
							break;
						}
					}
					for (unordered_map<string, Relation*>::iterator iit = store->begin(); iit!=store->end(); ++iit){
						itAtt[1] = iit->second->attributes->find(joinAtt2);
						if(itAtt[1] != iit->second->attributes->end()){
							itRel[1] = iit;
							break;
						}
					}
					Relation* joinedRel = new Relation();
					char * joinName = new char[200];
					sprintf(joinName, "%s|%s", itRel[0]->first.c_str(), itRel[1]->first.c_str());
					string joinNamestr(joinName);
					joinedRel->numTuples = tempRes;
					joinedRel->joinedRelationCount = numToJoin;
					for(int i = 0; i < 2; i++){
						for (unordered_map<string, int>::iterator iit = itRel[i]->second->attributes->begin(); iit!=itRel[i]->second->attributes->end(); ++iit){
							joinedRel->attributes->insert(*iit);
						}
						store->erase(itRel[i]);
					}
                    this->store->insert(std::make_pair(joinNamestr, joinedRel));
				}
				else{
					string seleAtt(orlist->left->left->value);
					unordered_map<string, int>::iterator itAtt;
					unordered_map<string, Relation*>::iterator itRel;
					for (unordered_map<string, Relation*>::iterator iit = store->begin(); iit!=store->end(); ++iit){
						itAtt = iit->second->attributes->find(seleAtt);
						if(itAtt != iit->second->attributes->end()){
							itRel = iit;
							break;
						}
					}
					itRel->second->numTuples = tempRes;
				}
				orlist = orlist->rightOr;
			}
		}
		andlist = andlist->rightAnd;
	}
}

double Statistics::Estimate(struct AndList *parseTree, char **relNames, int numToJoin)
{
    if (parseTree == NULL) {
        if (numToJoin > 1) return -1;
        return this->store->find(relNames[0])->second->numTuples;
    }
    struct AndList *andTree = parseTree;
	struct OrList *orlist = NULL;
	double result = 0.0, fraction = 1.0;
	int state = 0;

	while (andTree != NULL) {
		if (andTree->left != NULL){
			orlist = andTree->left;
			double fractionOr = 0.0;
			unordered_map<string, int>::iterator lastAtt;
			while (orlist != NULL){
				if (orlist->left->left->code == 3 && orlist->left->right->code == 3){
					unordered_map<string, int>::iterator itAtt[2];
					unordered_map<string, Relation*>::iterator itRel[2];

					for (unordered_map<string, Relation*>::iterator iit = this->store->begin(); iit != this->store->end(); ++iit){
						itAtt[0] = iit->second->attributes->find(orlist->left->left->value);
						if(itAtt[0] != iit->second->attributes->end()){
							itRel[0] = iit;
							break;
						}
					}
					for (unordered_map<string, Relation*>::iterator iit = this->store->begin(); iit != this->store->end(); ++iit){
						itAtt[1] = iit->second->attributes->find(orlist->left->right->value);
						if(itAtt[1] != iit->second->attributes->end()){
							itRel[1] = iit;
							break;
						}
					}
					
					double max;
					if (itAtt[0]->second >= itAtt[1]->second)		max = (double)itAtt[0]->second;
					else		max = (double)itAtt[1]->second;
					if (state == 0)
						result = (double)itRel[0]->second->numTuples*(double)itRel[1]->second->numTuples/max;
					else
						result = result*(double)itRel[1]->second->numTuples/max;
				
					state = 1;
				}
				else {
					unordered_map<string, int>::iterator itAtt;
					unordered_map<string, Relation*>::iterator itRel;
					for (unordered_map<string, Relation*>::iterator iit = store->begin(); iit!= store->end(); ++iit){
						itAtt = iit->second->attributes->find(orlist->left->left->value);
						if(itAtt != iit->second->attributes->end()){
							itRel = iit;
							break;
						}
					}
					if (result == 0.0)
						result = ((double)itRel->second->numTuples);
					double tempFrac;
					if(orlist->left->code == 7)
						tempFrac = 1.0 / itAtt->second;
					else
						tempFrac = 1.0 / 3.0;
					if(lastAtt != itAtt)
						fractionOr = tempFrac+fractionOr-(tempFrac*fractionOr);
					else
						fractionOr += tempFrac;
					lastAtt = itAtt;//
				}
				orlist = orlist->rightOr;
			}
			if (fractionOr != 0.0)
				fraction = fraction*fractionOr;
		}
		andTree = andTree->rightAnd;
	}
	result = result * fraction;
	tempRes = result;
	return result;
}

int Statistics::validateParams(struct AndList *parseTree, char **relNames, int numToJoin) {
    return 1;
}

void Statistics::printStore() {
    for (unordered_map<string, Relation*>::iterator iit = store->begin(); iit!=store->end(); ++iit) { 
        std::cout << iit->first << " " << *iit->second << "\n";
    }
}

