#ifndef BILSMTREE_HASH_H
#define BILSMTREE_HASH_H

namespace bilsmtree {

uint32_t Hash(const char* key, size_t len, uint32_t seed);

}

#endif