#ifndef BILSMTREE_TABLE_H
#define BILSMTREE_TABLE_H

#include <vector>
#include <string>

#include "bloomfilter.h"
#include "cuckoofilter.h"
#include "filesystem.h"

namespace bilsmtree {

struct Meta {
  Slice largest_;
  Slice smallest_;
  size_t sequence_number_;
  size_t level_;
  size_t file_size_;

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
  Block(const char* data, const size_t size) : data_(data), size_(size) { }
  
  ~Block() { }

  const char* data() { return data_; }

  size_t size() { return size_; }
private:
  const char *data_;
  size_t size_;
};

class Table {
public:
  Table();

  Table(const std::vector<KV>& kvs, FileSystem* filesystem);
  
  ~Table();

  Meta GetMeta();

  void DumpToFile(const std::string filename);
private:
  Block **data_blocks_;
  Block *index_block_;
  Filter *filter_;
  size_t data_block_number_;
  Meta meta_;
  FileSystem* filesystem_;
};


}

#endif