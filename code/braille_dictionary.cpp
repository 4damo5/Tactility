#include "braille_dictionary.h"

// Each letter is stored as 6 ints: row1col0, row1col1, row2col0, row2col1, row3col0, row3col1
// Indexed by: 0=a, 1=b, ... 25=z
static const int patterns[26][6] = {
    {1,0, 0,0, 0,0}, // a
    {1,0, 1,0, 0,0}, // b
    {1,1, 0,0, 0,0}, // c
    {1,1, 0,1, 0,0}, // d
    {1,0, 0,1, 0,0}, // e
    {1,1, 1,0, 0,0}, // f
    {1,1, 1,1, 0,0}, // g
    {1,0, 1,1, 0,0}, // h
    {0,1, 1,0, 0,0}, // i
    {0,1, 1,1, 0,0}, // j
    {1,0, 0,0, 1,0}, // k
    {1,0, 1,0, 1,0}, // l
    {1,1, 0,0, 1,0}, // m
    {1,1, 0,1, 1,0}, // n
    {1,0, 0,1, 1,0}, // o
    {1,1, 1,0, 1,0}, // p
    {1,1, 1,1, 1,0}, // q
    {1,0, 1,1, 1,0}, // r
    {0,1, 1,0, 1,0}, // s
    {0,1, 1,1, 1,0}, // t
    {1,0, 0,0, 1,1}, // u
    {1,0, 1,0, 1,1}, // v
    {0,1, 1,1, 0,1}, // w
    {1,1, 0,0, 1,1}, // x
    {1,1, 0,1, 1,1}, // y
    {1,0, 0,1, 1,1}, // z
};
    
const int* getBraillePattern(char c) {
    if (c >= 'a' && c <= 'z') return patterns[c - 'a'];
    if (c >= 'A' && c <= 'Z') return patterns[c - 'A'];
    return nullptr;
}

bool isUpper(char c){
    return (c >= 'A' && c <= 'Z');
} 
