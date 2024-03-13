//
// Created by ofir1 on 05-Mar-24.
//
#ifndef COMPARCH_HW2_UTILS_H
#define COMPARCH_HW2_UTILS_H

#include <vector>
#include <stdlib.h>
#include <memory>
#include <math.h>

using namespace std;

enum blockState {
    INVALID = 0,
    VALID = 1,
    DIRTY = 2,
};

class Block {
public:
    unsigned long int tag;
    blockState state;

    Block();
    ~Block() = default;
    Block(const Block& block)= default;
    Block &operator=(const Block& block)= default;
};


class Cache {
public:
    int sets;
    int blockSize;
    int ways;
    int cycles;
    bool writeAllocate;
    int tagSize;
    int cacheSize;
    int accessCnt;
    int missCnt;
    int height;
    vector<vector<shared_ptr<Block>>> blocksArr;
    vector<vector<int>> lruArr;


    Cache(int sets, int blockSize, int ways, int cycles, bool writeAllocate, int cacheSize);
    ~Cache() = default;
    Cache(const Cache& cache)= default;
    Cache &operator=(const Cache& cache)= default;

    blockState getState(int set, int way);
    int getWay(int set, unsigned long int pc);
    void updateLRU(int set, const int way);
    int getLRU(int set);
    unsigned long int calcTag(const unsigned long int pc);
    int calcSet(const unsigned long int pc);
    int findSpot(int set);
    bool cacheHit(const unsigned long int pc);
    void toDirty(const unsigned long int pc);
    void toInsert(const unsigned long int pc);
    void toRemove(int set, const int way);
};

//helper Functions
void exeCmd(char operation, unsigned long int pc, Cache* l1, Cache* l2);
void calcTime(Cache *l1, Cache* l2, double *l1missRate,
              double *l2missRate, double *avgTime,
              unsigned int l1Cycles, unsigned int l2Cycles,
              unsigned int memCycles);

#endif //COMPARCH_HW2_UTILS_H
