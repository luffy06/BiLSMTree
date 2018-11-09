#ifndef BILSMTREE_TABLE_H
#define BILSMTREE_TABLE_H

#include <vector>
#include <string>

#include "bloomfilter.h"
#include "cuckoofilter.h"
#include "filesystem.h"

namespace bilsmtree {

class Filter;
class BloomFilter;
class CuckooFilter;
class FileSystem;

struct Meta {
  Slice largest_;
  Slice smallest_;
  uint sequence_number_;
  uint level_;
  uint file_size_;

  Meta() {
    
  }

  bool Intersect(Meta other) const {
    if (largest_.compare(other.smallest_) >= 0 && smallest_.compare(other.largest_) <= 0)
      return true;
    return false;
  }
};

class Block {
public:
  Block(const char* data, const uint size) : data_(data), size_(size) { }
  
  ~Block();

  const char* data() { return data_; }

  uint size() { return size_; }
private:
  const char *data_;
  uint size_;
};

class Table {
public:
  Table();

  Table(const std::vector<KV>& kvs);
  
  ~Table();

  Meta GetMeta();

  void DumpToFile(const std::string filename);
private:
  Block **data_blocks_;
  Block *index_block_;
  Filter *filter_;
  uint data_block_number_;
  Meta meta_;
};


}

#endif