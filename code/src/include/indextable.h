#ifndef BILSMTREE_INDEXTABLE_H
#define BILSMTREE_INDEXTABLE_H

#include <vector>
#include <string>
#include <sstream>

#include "table.h"

namespace bilsmtree {

struct BlockMeta {
  Slice smallest_;
  Slice largest_;
  size_t offset_;
  size_t block_size_;
  size_t block_numb_;
  Filter* filter_;

  BlockMeta() {
    filter_ = NULL;
  }

  ~BlockMeta() {
  }

  std::string ToString() const {
    std::stringstream ss;
    ss << smallest_.ToString();
    ss << Config::DATA_SEG;
    ss << largest_.ToString();
    ss << Config::DATA_SEG;
    ss << offset_;
    ss << Config::DATA_SEG;
    ss << block_size_;
    ss << Config::DATA_SEG;
    ss << block_numb_;
    ss << Config::DATA_SEG;    
    return ss.str();
  }

  BlockMeta& operator=(const BlockMeta& bm) {
    smallest_ = Slice(bm.smallest_.data(), bm.smallest_.size());
    largest_ = Slice(bm.largest_.data(), bm.largest_.size());
    offset_ = bm.offset_;
    block_size_ = bm.block_size_;
    block_numb_ = bm.block_numb_;
    filter_ = bm.filter_;
    return *this;
  }

};

class IndexTable : public Table {
public:
  IndexTable();

  IndexTable(const std::vector<BlockMeta>& bms, size_t sequence_number, std::string filename, FileSystem* filesystem, LSMTreeResult* lsmtreeresult);
  
  ~IndexTable();
};


}

#endif