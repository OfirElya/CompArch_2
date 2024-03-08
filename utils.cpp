//
// Created by ofir1 on 05-Mar-24.
//
#include "utils.h"

#include <cassert>

///////////////// Block methods /////////////////
Block::Block(int blockSize) : state(INVALID) {
    int vecSize = blockSize/4;
    for(int i=0; i<vecSize; i++){
        data.push_back(0);
    }
}




///////////////// Cache methods /////////////////

Cache::Cache(int sets, int blockSize, int ways, int cycles, bool writeAllocate, int cacheSize) : sets(sets), blockSize(blockSize), ways(ways), cycles(cycles), writeAllocate(writeAllocate), cacheSize(cacheSize){
    // Initialize lruArray
    for(int i=0; i<sets; i++){
        vector<int> innerVec;
        for(int j=0; j<ways; j++){
            innerVec.push_back(j);
        }
        lruArr.push_back(innerVec);
    }

    // Initialize blocksArray
    for(int i=0; i<ways; i++){
        vector<unique_ptr<Block>> innerVec;
        for(int j=0; j<sets; j++){
            innerVec.push_back(unique_ptr<Block>(new Block(blockSize)));
        }
        blocksArr.push_back(move(innerVec));
    }
}





int Cache::calcTag(const int pc){
    return ( (pc >> (32 - this->tagSize)) );
}

//bool Cache::isValid(const int tag, const int set) {
//
//}

void Cache::updateLRU(const int set, const int way) {
    int wayCnt = lruArr[set][way];
    lruArr[set][way] = this->ways;
    for(int i =0;i<this->ways; i++){
        if(i == way)
            continue;
        if(lruArr[set][i] > wayCnt)
            lruArr[set][i]--;
    }
}

int Cache::getLRU(const int set){
    for(int i =0;i<this->ways;i++){
        if(lruArr[set][i] == 0)
            return i;
    }
    return 0;
}

void Cache::replaceBlock(Block *block) {

}