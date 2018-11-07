#ifndef BILSMTREE_BLOOMFILTER_H
#define BILSMTREE_BLOOMFILTER_H

#include <vector>
#include <string>

namespace bilsmtree {

class Slice;
class Filter;
class Util;

class BloomFilter : Filter {
public:
  BloomFilter() { }

  BloomFilter(const std::vector<Slice>& keys);

  BloomFilter(std::string data);
  
  ~BloomFilter();

private:
  const size_t k_;

  char *array_;
  size_t bits_;

  void Add(const Slice& key);
};
}

#endif