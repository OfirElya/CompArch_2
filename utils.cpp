//
// Created by ofir1 on 05-Mar-24.
//
#include "utils.h"

#include <cassert>

///////////////// Block methods /////////////////
Block::Block() : tag(-1), state(INVALID) {}

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
        vector<shared_ptr<Block>> innerVec;
        for(int j=0; j<sets; j++){
            innerVec.push_back(shared_ptr<Block>(new Block()));
        }
        blocksArr.push_back(innerVec);
    }
}

blockState Cache::getState(int set, int way){
    return this->blocksArr[set][way]->state;
}

// check if tag exists in set
int Cache::getWay(int set, int tag){
    for (int i = 0; i < this->ways; i++){
        if (this->blocksArr[set][i]->tag == tag)
        return i;
    }
    return this->ways;
}

void Cache::insertData(int tag, int set, int way){

    if (this->blocksArr[set][way]->state != INVALID){
        this->blocksArr[set][way]->state = DIRTY;
    }
    else {
        this->blocksArr[set][way]->state = VALID;
    }
    this->blocksArr[set][way]->tag = tag;
    updateLRU(set, way);

    return;
    
}

int Cache::calcTag(const int pc){
    return ( (pc >> (32 - this->tagSize)) );
}

int Cache::calcSet(const int pc){
    return ((pc >> (this->blockSize) % this->sets));
}


void Cache::updateLRU(const int set, const int way) {
    int way_accessed = lruArr[set][way];
    lruArr[set][way] = this->ways;
    for(int i =0;i<this->ways; i++){
        if(i == way)
            continue;
        if(lruArr[set][i] > way_accessed)
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

///////////////// helper functions /////////////////

// L2 snoops L1 - return true if tag exists in L1 and isn't INVALID, update the location of found tag in L1
//FIXME: update function
bool snoop (int tag, Cache* L1, int* L1_set, int* L1_way){
	for (int set = 0; set < L1->sets; set++)
    {
        for (int way = 0; way < L1->ways; way++)
        {
            if (L1->blocksArr[set][way]->tag == tag && L1->blocksArr[set][way]->state != INVALID)
            {
                *L1_set = set;
                *L1_way = way;
                return true;
            }
	    }
    }
	return false;
}

// if the specified set in Cache C has a free space, replace it with wanted tag.
// return true if replaced, false if there was no space to insert data
bool tryReplace(Cache* C, int tag, int set){
    for (int i = 0; i < C->ways; i++){
        if (C->getState(set, i) == INVALID){
            C->insertData(tag, set, i);
            return true;
        }
    }
    return false;
}

// rebuild pc from tag and set. assuming same block_size in all caches and ignoring block offset
int buildPc(int tag, int set, int block_size, int sets_num){
    int sets_size = log2(sets_num);
    int pc = (tag << (block_size + sets_size)) | (set << sets_size);
    return pc;
}

void writeMiss(int pc, Cache* L1,  Cache* L2){
    if (!L1->writeAllocate) return;

    int L1_tag = L1->calcTag(pc);
    int L2_tag = L2->calcTag(pc);
    int L1_set = L1->calcSet(pc);
    int L2_set = L2->calcSet(pc);

    bool L1_replaced = false;
    bool L2_replaced = false;

    L2_replaced = tryReplace(L2, L2_tag, L2_set);

    if (L2_replaced){
        L1_replaced = tryReplace(L1, L1_tag, L1_set);
        if (L1_replaced) return;

        // inserted data to L2 but no space in L1
        int lru_way = L1->getLRU(L1_set);
        blockState lru_state = L1->getState(L1_set, lru_way);
       
        // way is dirty - need to writeback to L2 before replacing
        if (lru_state == DIRTY){
            writeBack(L1_set, lru_way, L1, L2);
        }
        // invalidate replaced block in L1
        L1->blocksArr[L1_set][lru_way]->state = INVALID;
        // write new block to L1
        L1->insertData(L1_tag, L1_set, lru_way);
        return;
    }
    else {
        int lru_way = L2->getLRU(L2_set);
        //snoop
        int L2_tag_snooped = L2->blocksArr[L2_set][lru_way]->tag;
        int L1_pc_snooped = buildPc(L2_tag_snooped, L2_set, L2->blockSize, L2->sets);
        int L1_set_snooped = L1->calcSet(L1_pc_snooped);
        int L1_tag_snooped = L1->calcTag(L1_pc_snooped);
        int L1_way_snooped = L1->getWay(L1_set_snooped, L1_tag_snooped);
       
       // tag isn't in L1 - we can remove it from L2 safely
        if (L1_way_snooped == L1->ways){
            // invalidate replaced block in L2
            L2->blocksArr[L2_set][lru_way]->state = INVALID;
            // write new block to L2
            L2->insertData(L2_tag, L2_set, lru_way);
        }

        // tag exists in L1 - need to update it in L2 and invalidate in L1 before removing from L2
        else {
            blockState snooped_state = L1->getState(L1_set_snooped, L1_way_snooped);
            if (snooped_state == DIRTY){
                writeBack(L1_set_snooped, L1_way_snooped, L1, L2);
            }
            // invalidate replaced block in L1
            L1->blocksArr[L1_set_snooped][L1_way_snooped]->state = INVALID;
            // write new block to L2
            L2->insertData(L2_tag, L2_set, lru_way);
        } 
        // done with snoop - wrote new data to L2

        // try to write new block to L1
        L1_replaced = tryReplace(L1, L1_tag, L1_set);
        if (L1_replaced) return;

        // inserted data to L2 but no space in L1
        int lru_way = L1->getLRU(L1_set);
        blockState lru_state = L1->getState(L1_set, lru_way);
       
        // way is dirty - need to writeback to L2 before replacing
        if (lru_state == DIRTY){
            writeBack(L1_set, lru_way, L1, L2);
        }
        // invalidate replaced block in L1
        L1->blocksArr[L1_set][lru_way]->state = INVALID;
        // write new block to L1
        L1->insertData(L1_tag, L1_set, lru_way);
        return;
    }
}

void writeHitL1(int pc, Cache* L1){
    int tag = L1->calcTag(pc);
    int set = L1->calcSet(pc);
    int way = L1->getWay(set, tag);

    L1->blocksArr[set][way]->state = DIRTY;
    L1->updateLRU(set, way);
    return;
}

void writeHitL2(int pc, Cache* L2, Cache* L1){
    int L1_tag = L1->calcTag(pc);
    int L2_tag = L2->calcTag(pc);
    int L1_set = L1->calcSet(pc);
    int L2_set = L2->calcSet(pc);
    int L2_way = L2->getWay(L2_set, L2_tag);

    //update in L2
    L2->blocksArr[L2_set][L2_way]->state = DIRTY;
    L2->updateLRU(L2_set, L2_way);

    if (!L2->writeAllocate) return;
    else {
        // try replacing in L1
        bool L1_replaced = tryReplace(L1, L1_tag, L1_set);
        if (L1_replaced) return;
        else { 
            // inserted data to L2 but no space in L1
            int lru_way = L1->getLRU(L1_set);
            blockState lru_state = L1->getState(L1_set, lru_way);
        
            // way is dirty - need to writeback to L2 before replacing
            if (lru_state == DIRTY){
                writeBack(L1_set, lru_way, L1, L2);
            }
            // invalidate replaced block in L1
            L1->blocksArr[L1_set][lru_way]->state = INVALID;
            // write new block to L1
            L1->insertData(L1_tag, L1_set, lru_way);
            return;
        }
    }
}

void writeBack(int L1_set, int lru_way, Cache* L1, Cache* L2){
    int L1_tag_replaced = L1->blocksArr[L1_set][lru_way]->tag;
    int L2_pc_dirty = buildPc(L1_tag_replaced, L1_set, L1->blockSize, L1->sets);
    int L2_set_dirty = L2->calcSet(L2_pc_dirty);
    int L2_tag_dirty = L2->calcTag(L2_pc_dirty);
    int L2_way_dirty = L2->getWay(L2_set_dirty, L2_tag_dirty);
    L2->insertData(L2_tag_dirty, L2_set_dirty, L2_way_dirty);
}

void readMiss(){
    // try insert to L2 and L1
    return;
}

void readHitL2(){
    // update LRU in L2
    // try insert to L1
    return;
}

void exeCmd(char operation, unsigned long int pc, Cache* L1, Cache* L2){
    int L1_tag = L1->calcTag(pc);
    int L2_tag = L2->calcTag(pc);
    int L1_set = L1->calcSet(pc);
    int L2_set = L2->calcSet(pc);
    int L1_way = L1->getWay(L1_set, L1_tag);
    int L2_way = L2->getWay(L2_set, L2_tag);

    if (operation == 'R'){
        if (L1_way < L1->ways && L1->getState(L1_set, L1_way) != INVALID){
            //read hit
            L1->updateLRU(L1_set, L1_way);
            return;
        }
        else if (L2_way < L2->ways && L2->getState(L2_set, L2_way) != INVALID){
            //read hit L2

        }
        else {
            //read miss
        }
    }
    if (operation == 'W'){
        if (L1_way < L1->ways && L1->getState(L1_set, L1_way) != INVALID){
            // write hit L1
            writeHitL1(pc, L1);
            return;
        }
        else if (L2_way < L2->ways && L2->getState(L2_set, L2_way) != INVALID){
            // write hit L2
            writeHitL2(pc, L2, L1);
            return;
        }
        else {
            // write miss
            writeMiss(pc, L1, L2);
            return;
        }
    }
}

