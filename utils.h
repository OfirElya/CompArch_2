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
    MODIFIED = 3
};




class Block {
private:
    // tag starts as -1
public:
    int tag;
    blockState state;
    vector<int> data;


    Block(int blockSize);
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
    int tagSize;
    int cacheSize;
    vector<vector<unique_ptr<Block>>> blocksArr;
    vector<vector<int>> lruArr;


    Cache(int sets, int blockSize, int ways, int cycles, bool writeAllocate, int cacheSize);
    ~Cache() = default;
    Cache(const Cache& cache)= default;
    Cache &operator=(const Cache& cache)= default;

    void replaceBlock(Block* block);
    void insertData(int tag, int set,int offset, int data);
    bool isDirty(const int tag, const int set);
    bool isValid(const int tag, const int set);
    void updateLRU(const int set, const int way);
    int getLRU(const int set);
    int calcTag(const int pc);
};


#endif //COMPARCH_HW2_UTILS_H
