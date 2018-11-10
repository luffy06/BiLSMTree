#ifndef BILSMTREE_FILTER_H
#define BILSMTREE_FILTER_H

#include <string>

#include "slice.h"
#include "hash.h"
#include "util.h"

namespace bilsmtree {

class Slice;

class Filter {
public:
  Filter() { }

  virtual ~Filter() { }

  virtual bool KeyMatch(const Slice key);

  virtual std::string ToString();
};
}

#endif