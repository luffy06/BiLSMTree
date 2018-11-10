#ifndef BILSMTREE_HASH_H
#define BILSMTREE_HASH_H

#include <iostream>
#include <cstring>

namespace bilsmtree {

size_t Hash(const char* key, size_t len, size_t seed);

}

#endif