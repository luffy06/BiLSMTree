#include "hash.h"

namespace bilsmtree {

size_t Hash(const char* data, size_t n, size_t seed) {
  // Similar to murmur hash
  const size_t m = 0xc6a4a793;
  const size_t r = 24;
  const char* limit = data + n;
  size_t h = seed ^ (n * m);

  // Pick up four bytes at a time
  while (data + 4 <= limit) {
    size_t w;
    memcpy(&w, data, sizeof(w));  // gcc optimizes this to a plain load
    data += 4;
    h += w;
    h *= m;
    h ^= (h >> 16);
  }

  // Pick up remaining bytes
  switch (limit - data) {
    case 3:
      h += static_cast<unsigned char>(data[2]) << 16;
    case 2:
      h += static_cast<unsigned char>(data[1]) << 8;
    case 1:
      h += static_cast<unsigned char>(data[0]);
      h *= m;
      h ^= (h >> r);
      break;
  }
  return h;

}

}