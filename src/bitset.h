#ifndef _BITSET_H_
#define _BITSET_H_
#include <iostream>
#include <vector>
using namespace std;



//定义了位图数据结构
template <size_t N>
class bitset{
    public:
    bitset(){
        _bits.resize(N/8 + 1,0); //包括0
    }
    void set(size_t x){
        size_t i = x / 8;
        size_t j = x % 8;
        _bits[i] |= (1 << j);
    }

    void reset(size_t x){
        size_t i = x / 8;
        size_t j = x % 8;
        _bits[i] &= ~(1 << j);
    }

    bool isExists(size_t x){
        size_t i = x / 8;
        size_t j = x % 8;
        return _bits[i] & (1 << j);
    }
    private:
        vector<char> _bits;
};
#endif
