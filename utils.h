//
// Created by ofir1 on 05-Mar-24.
//

#include <vector>
#include <stdlib.h>
#include <memory>


#ifndef COMPARCH_HW2_UTILS_H
#define COMPARCH_HW2_UTILS_H

using namespace std;

enum blockState {
    INVALID = 0,
    VALID = 1,
    DIRTY = 2,
} blockState_e;


class Block {
private:
    // tag starts as -1
public:
    int tag;
    blockState state;

    Block();
    ~Block() = default;
    Block(const Block& block)= default;
    Block &operator=(const Block& block)= default;
};


class Cache {
private:
public:
    int sets;
    int blockSize;
    int ways;
    int cycles;
    bool writeAllocate;
    bool L2;
    int tagSize;
    int cacheSize;
    vector<vector<shared_ptr<Block>>> blocksArr;
    vector<vector<int>> lruArr;


    Cache(int sets, int blockSize, int ways, int cycles, bool writeAllocate, bool L2, int cacheSize);
    ~Cache() = default;
    Cache(const Cache& cache)= default;
    Cache &operator=(const Cache& cache)= default;

    blockState getState(int set, int way);
    int getWay(int set, int tag);
    void insertData(int tag, int set, int way);
    void updateLRU(const int set, const int way);
    int getLRU(const int set);
    int calcTag(const int pc);
    int calcSet(const int pc);
};

//helper Functions
bool snoop (int tag, int set, Cache* L1);
int buildPc(int tag, int set, int block_size, int sets_num);
bool tryReplace(Cache* C, int tag, int set);
void writeMiss(int pc, Cache* L1,  Cache* L2);


#endif //COMPARCH_HW2_UTILS_H
