#ifndef BILSMTREE_HASH_H
#define BILSMTREE_HASH_H

#include <iostream>

namespace bilsmtree {

uint Hash(const char* key, uint len, size_t seed);

}

#endif