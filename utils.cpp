//
// Created by ofir1 on 05-Mar-24.
//
#include "utils.h"

#include <cassert>


void writeData(const int tag, const int set, const int way, Cache* cache);
///////////////// Block methods /////////////////
Block::Block() : tag(-1), state(INVALID) {}

///////////////// Cache methods /////////////////

Cache::Cache(int sets, int blockSize, int ways, int cycles, bool writeAllocate, int cacheSize) : sets(sets), blockSize(blockSize), ways(ways), cycles(cycles), writeAllocate(writeAllocate), cacheSize(cacheSize){
    // Initialize lruArray
    height = (int) pow(2,(cacheSize - blockSize - log2(ways)));
    sets = height;
    for(int i=0; i<sets; i++){
        vector<int> innerVec;
        for(int j=0; j<ways; j++){
            innerVec.push_back(j);
        }
        lruArr.push_back(innerVec);
    }

    // Initialize blocksArray
    for(int i=0; i<sets; i++){
        vector<shared_ptr<Block>> innerVec;
        for(int j=0; j<ways; j++){
            innerVec.push_back(shared_ptr<Block>(new Block()));
        }
        blocksArr.push_back(innerVec);
    }
    accessCnt = 0;
    missCnt = 0;
}

blockState Cache::getState(int set, int way){
    return blocksArr[set][way]->state;
}

// check if tag exists in set
int Cache::getWay(int set, unsigned long int pc){
    for (int i = 0; i < this->ways; i++){
        if ((blocksArr[set][i]->tag >> blockSize) == (pc >> blockSize))
            return i;
    }
    return this->ways;
}

bool Cache::cacheHit(const unsigned long int pc){
    int set = calcSet(pc);
    for (int i = 0; i < this->ways; i++){
        if ((blocksArr[set][i]->tag >> blockSize) == (pc >> blockSize))
            return true;
    }
    return false;
}

void Cache::toDirty(const unsigned long int pc){
    int set = calcSet(pc);
    int i;
    for ( i = 0; i < this->ways; i++){
        if ( (blocksArr[set][i]->tag >> blockSize) == (pc >> blockSize)){
            blocksArr[set][i]->state = DIRTY;
            break;
        }
    }
    this->updateLRU(set, i);
}

void Cache::toInsert(const unsigned long int pc){
    int set = this->calcSet(pc);
    int i;
    for ( i = 0; i < this->ways; i++){
        if (this->blocksArr[set][i]->state == INVALID) {
            this->blocksArr[set][i]->state = VALID;
            this->blocksArr[set][i]->tag = pc;
            break;
        }
    }
    this->updateLRU(set, i);
}

void Cache::toRemove(const int set, const int way){
    this->blocksArr[set][way]->tag = -1;
    this->blocksArr[set][way]->state = INVALID;
//    updateLRU(set, way);
}

int Cache::findSpot(int set){
    for (int i = 0; i < this->ways; i++){
        if (this->blocksArr[set][i]->state == INVALID)
            return i;
    }
    return -1;
}

unsigned long int Cache::calcTag(const unsigned long int pc){
    return ( (pc >> (32 - this->tagSize)) );
}

int Cache::calcSet(const unsigned long int pc){
    return ((pc >> (this->blockSize) ) % this->height);
}

void Cache::updateLRU(const int set, const int way) {
    int way_accessed = this->lruArr[set][way];
    this->lruArr[set][way] = this->ways - 1;
    for(int i =0;i<this->ways; i++){
        if(i == way)
            continue;
        if(this->lruArr[set][i] > way_accessed)
            this->lruArr[set][i]--;
    }
}

int Cache::getLRU(const int set){
    for(int i =0;i<this->ways;i++){
        if(this->lruArr[set][i] == 0)
            return i;
    }
    return this->ways;
}

///////////////// helper functions /////////////////





//////////////////////////////// OFIR CHANGES /////////////////////////////////



void exeCmdNew(char operation, unsigned long int pc, Cache* l1, Cache* l2) {
    int l1Set = l1->calcSet(pc);
    int l2Set = l2->calcSet(pc);
    bool l1Hit = l1->cacheHit(pc);
    l1->accessCnt++;
    if(operation == 'r' || (operation == 'w' && l1->writeAllocate)){
        if(!l1Hit){
            // Missed L1 , Accessed L2
            l1->missCnt++;
            l2->accessCnt++;
            bool l2Hit = l2->cacheHit(pc);

            if(!l2Hit){
                l2->missCnt++;

                ///////// TRY INSERT TO L2 START /////////
                int l2spot = l2->findSpot(l2Set);
                if(l2spot == -1) {
                    int lruWay = l2->getLRU(l2Set);
                    unsigned long int lruTag = l2->blocksArr[l2Set][lruWay]->tag;
                    l2->toRemove(l2Set, lruWay);//TODO do i need to update lru? NO
                    //NOA: FIRST SNOOP TO L1, THEN REMOVE FROM L2

                    //remove from l1 if exists
                    if( l1->cacheHit(lruTag)){
                        int lruL1Set = l1->calcSet(lruTag);
                        int lru1Way = l1->getWay(lruL1Set, lruTag);
                        l1->toRemove(lruL1Set, lru1Way);//TODO: do i need to update lru?
                    }
                }
                l2->toInsert(pc);
                if(operation == 'w'){
                    int insertWay = l2->getWay(l2Set, pc);
                    l2->blocksArr[l2Set][insertWay]->state = DIRTY;
                }
                ///////// TRY INSERT TO L2 END
            }
            else if(operation == 'r'){
                int l2Way = l2->getWay(l2Set, pc);
                l2->updateLRU(l2Set, l2Way);
            }


            ///////// TRY INSERT TO L1 START /////////
            int l1spot = l1->findSpot(l1Set);
            if( l1spot == -1) {
                int lruWay = l1->getLRU(l1Set);
                unsigned long int lruTag = l1->blocksArr[l1Set][lruWay]->tag;
                blockState lruState = l1->blocksArr[l1Set][lruWay]->state;
                l1->toRemove(l1Set, lruWay);
                // if dirty, write dirty in l2
                if( lruState == DIRTY){
                    int lruL2Set = l2->calcSet(lruTag);
                    int lru2Way = l2->getWay(lruL2Set, lruTag);
                    l2->toDirty(lruTag);
                }
            }
            l1->toInsert(pc);
            if(operation == 'w'){
                int insertWay = l1->getWay(l1Set, pc);
                l1->blocksArr[l1Set][insertWay]->state = DIRTY;
            }
            ///////// TRY INSERT TO L1 END
        }
        // If the operation is write, i want to make the tag dirty
        else if(operation == 'w')
            l1->toDirty(pc);
        // if hit on read, just update lru
        else if(operation == 'r')
            l1->updateLRU(l1Set, l1->getWay(l1Set, pc));

    }
    else if( operation == 'w' && !l1->writeAllocate){
        if (l1Hit)
            l1->toDirty(pc);
        else{
            l1->missCnt++;
            l2->accessCnt++;
            if(!l2->cacheHit(pc))
                l2->missCnt++;
            else l2->toDirty(pc);
        }
    }
}

void calcTime(Cache *l1, Cache* l2, double *l1missRate,
              double *l2missRate, double *avgTime,
              unsigned int l1Cycles, unsigned int l2Cycles,
              unsigned int memCycles)
{
    *l1missRate = (double) l1->missCnt / (double) l1->accessCnt;
    *l2missRate = (double) l2->missCnt / (double) l2->accessCnt;
    *avgTime = (double) ((l1->accessCnt * l1Cycles) +
                         (l2->accessCnt * l2Cycles) + (l2->missCnt * memCycles)) /
               (double) l1->accessCnt;
}

unsigned long int buildPc(int tag, int set, int block_size, int sets_num){
    int sets_size = log2(sets_num);
    unsigned long int pc = (tag << (block_size + sets_size)) | (set << sets_size);
    return pc;
}


// I WANT LRU TO RETURN ME THE pc of the