
// Created by ofir1 on 07-Mar-24.


//
// Created by ofir1 on 23-May-23.
//


#include "utils.h"
#include <iostream>


using namespace std;

int main() {
    Block k(8);
    k.tag = 5;
    cout << "tag == " << k.tag << ", state == " << k.state << endl;
    for(int i = 0; i< 8/4; i++){
        cout<< "data[" << i << "] == " << k.data[i] << endl;
    }
    Cache l1(2,8,2,0,1,8);
    cout << l1.blocksArr[0][0]->state << endl;
    unique_ptr<Block> p = unique_ptr<Block>(new Block(5));
//    free(p);
    return 0;
}
