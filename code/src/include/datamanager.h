#ifndef BILSMTREE_DATAMANAGER_H
#define BILSMTREE_DATAMANAGER_H

#include "indextable.h"

namespace bilsmtree {

class DataManager {
public:
  DataManager(FileSystem *filesystem);

  ~DataManager();
  
  std::vector<BlockMeta> Append(const std::vector<KV>& kvs);

  bool Get(const Slice key, Slice& value, size_t offset, size_t block_size);

  std::string ReadBlock(const BlockMeta& bm);

  void Invalidate(const BlockMeta& bm);
private:
  size_t file_size_;
  size_t total_block_number_;
  std::vector<size_t> status_;
  FileSystem *filesystem_;

  std::string GetFilename();

  // void CollectGarbage();

  std::vector<BlockMeta> WriteBlocks(const std::vector<std::string>& block_datas);
};
}

#endif