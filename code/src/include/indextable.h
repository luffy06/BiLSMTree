#ifndef BILSMTREE_INDEXTABLE_H
#define BILSMTREE_INDEXTABLE_H

#include <vector>
#include <string>

#include "table.h"

namespace bilsmtree {

struct BlockMeta {
  Slice smallest_;
  Slice largest_;
  size_t file_numb_;
  size_t offset_;
  size_t block_size_;
  size_t block_numb_;
  Filter* filter_;

  BlockMeta() {
    filter_ = NULL;
  }

  std::string ToString() {
    std::stringstream ss;
    ss << smallest_.ToString();
    ss << Config::DATA_SEG;
    ss << largest_.ToString();
    ss << Config::DATA_SEG;
    ss << file_numb_;
    ss << Config::DATA_SEG;
    ss << offset_;
    ss << Config::DATA_SEG;
    ss << block_size_;
    ss << Config::DATA_SEG;
    ss << block_numb_;
    ss << Config::DATA_SEG;    
    return ss.str();
  }
};

class IndexTable : public Table {
public:
  IndexTable();

  IndexTable(const std::vector<KV>& kvs, size_t sequence_number, std::string filename, FileSystem* filesystem, LSMTreeResult* lsmtreeresult, DataManager* datamanager);
  
  ~IndexTable();

  Meta GetMeta();

private:
  Meta meta_;
};


}

#endif