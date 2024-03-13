//
// Created by ofir1 on 05-Mar-24.
//
#include "utils.h"

#include <cassert>

///////////////// Block methods /////////////////

// block constructor - initialize tag to -1 and state to invalid
Block::Block() : tag(-1), state(INVALID) {}

///////////////// Cache methods /////////////////

// cache constructor
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

// return state of specified block
blockState Cache::getState(int set, int way){
    return this->blocksArr[set][way]->state;
}

// if the tag exists in the set, return the way it's placed in. else return the number of ways
int Cache::getWay(int set, unsigned long int pc){
    for (int i = 0; i < this->ways; i++){
        if ((blocksArr[set][i]->tag >> blockSize) == (pc >> blockSize))
            return i;
    }
    return this->ways;
}

// return true if tag exists in cache, false else
bool Cache::cacheHit(const unsigned long int pc){
    int set = calcSet(pc);
    for (int i = 0; i < this->ways; i++){
        if ((this->blocksArr[set][i]->tag >> blockSize) == (pc >> blockSize) && this->blocksArr[set][i]->state != INVALID)
            return true;
    }
    return false;
}

// change the block state to dirty and update lru
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

// insert new block to available way in set and update lru
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
    return;
}

// remove block from set
void Cache::toRemove(const int set, const int way){
    this->blocksArr[set][way]->tag = -1;
    this->blocksArr[set][way]->state = INVALID;
    return;
}

// find the first available way in the set. if none, return -1
int Cache::findSpot(int set){
    for (int i = 0; i < this->ways; i++){
        if (this->blocksArr[set][i]->state == INVALID)
            return i;
    }
    return -1;
}

// calc tag from pc
unsigned long int Cache::calcTag(const unsigned long int pc){
    return ( (pc >> (32 - this->tagSize)) );
}

// calc set from pc
int Cache::calcSet(const unsigned long int pc){
    return ((pc >> (this->blockSize) ) % this->height);
}

// update the lru according to the way and set that were accessed
void Cache::updateLRU(const int set, const int way) {
    int way_accessed = this->lruArr[set][way];
    this->lruArr[set][way] = this->ways - 1;
    for(int i =0; i<this->ways; i++){
        if(i == way)
            continue;
        if(this->lruArr[set][i] > way_accessed)
            this->lruArr[set][i]--;
    }
    return;
}

// get the lru way in specified set
int Cache::getLRU(const int set){
    for(int i =0;i<this->ways;i++){
        if(this->lruArr[set][i] == 0)
            return i;
    }
    return this->ways;
}

///////////////// helper functions /////////////////

void exeCmd(char operation, unsigned long int pc, Cache* l1, Cache* l2) {
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
                // missed in l2
                l2->missCnt++;

                ///////// TRY INSERT TO L2 START /////////
                int l2spot = l2->findSpot(l2Set);
                if(l2spot == -1) {
                    // need to free space in l2
                    int lruWay = l2->getLRU(l2Set);
                    unsigned long int lruTag = l2->blocksArr[l2Set][lruWay]->tag;
                    l2->toRemove(l2Set, lruWay);

                    //remove from l1 if exists
                    if( l1->cacheHit(lruTag)){
                        int lruL1Set = l1->calcSet(lruTag);
                        int lru1Way = l1->getWay(lruL1Set, lruTag);
                        l1->toRemove(lruL1Set, lru1Way);
                    }
                }
                l2->toInsert(pc);
                if(operation == 'w'){
                    // after writing to block, change it to dirty
                    int insertWay = l2->getWay(l2Set, pc);
                    l2->blocksArr[l2Set][insertWay]->state = DIRTY;
                }
                ///////// TRY INSERT TO L2 END
            }
            else {
                // l2 hit - update l2 lru
                int l2Way = l2->getWay(l2Set, pc);
                l2->updateLRU(l2Set, l2Way);
            }


            ///////// TRY INSERT TO L1 START /////////
            int l1spot = l1->findSpot(l1Set);
            if( l1spot == -1) {
                // need to free space in l1
                int lruWay = l1->getLRU(l1Set);
                unsigned long int lruTag = l1->blocksArr[l1Set][lruWay]->tag;
                blockState lruState = l1->blocksArr[l1Set][lruWay]->state;
                l1->toRemove(l1Set, lruWay);
                // if dirty, write dirty in l2 and update l2 lru
                if( lruState == DIRTY){
                    l2->toDirty(lruTag);
                }
            }
            l1->toInsert(pc);
            if(operation == 'w'){
                // after writing to block, change it to dirty
                int insertWay = l1->getWay(l1Set, pc);
                l1->blocksArr[l1Set][insertWay]->state = DIRTY;
            }
            ///////// TRY INSERT TO L1 END
        }
        // L1 HIT
        // If the operation is write, make the tag dirty and update l1 lru
        else if(operation == 'w')
            l1->toDirty(pc);
        // If the operation is read, just update l1 lru
        else if(operation == 'r')
            l1->updateLRU(l1Set, l1->getWay(l1Set, pc));

    }
    else if( operation == 'w' && !l1->writeAllocate){
        // if write hit in l1, change to dirty and update lru
        if (l1Hit)
            l1->toDirty(pc);
        else{
            l1->missCnt++;
            l2->accessCnt++;
            // if missed in l2, just add to miss count (access memory)
            if(!l2->cacheHit(pc))
                l2->missCnt++;
            // if hit in l2, change it to dirty and update lru. no need to write to l1.
            else l2->toDirty(pc);
        }
    }
}

// calculate miss rates and avrage access time
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