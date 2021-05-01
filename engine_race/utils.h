#ifndef ENGINE_RACE_ENGINE_UTILS_H_
#define ENGINE_RACE_ENGINE_UTILS_H_

#include "include/engine.h"

#define inline inline __attribute__((always_inline))

typedef unsigned char u8;
typedef unsigned int u32;

typedef char i8;
typedef int i32;

const int MAX_KEY_LENGTH = 16;
const int INITIAL_FILE_SIZE = 8 * 1024 * 1024;
const int HASH_SIZE = 256;
const int HASH_SIZE_LOG = __builtin_ctz(HASH_SIZE);
const int M = 16;

enum TreeType
{
    AVL_,
    BPULS_
};

const TreeType TREE_TYPE = BPULS_;

inline int hash(const i8 *c)
{
    return (u8)(((u8)c[0]) >> (8 - HASH_SIZE_LOG));
}

#endif
