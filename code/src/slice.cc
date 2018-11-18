#include "slice.h"

namespace bilsmtree {

int Slice::compare(const Slice b) const {
  const size_t min_len = (size_ < b.size()) ? size_ : b.size();
  int r = memcmp(data_, b.data(), min_len);
  if (r == 0) {
    if (size_ < b.size()) r = -1;
    else if (size_ > b.size()) r = +1;
  }
  return r;
}

}