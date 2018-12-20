#ifndef BILSMTREE_DATAMANAGER_H
#define BILSMTREE_DATAMANAGER_H

#include "indextable.h"

namespace bilsmtree {

class DataManager {
public:
  DataManager(FileSystem *filesystem);

  ~DataManager();
  
  BlockMeta Append(const std::vector<KV>& kvs);

  bool Get(const Slice key, Slice& value, size_t file_numb, size_t offset, size_t block_size);

  std::string ReadBlock(const BlockMeta& bm);

  void Invalidate(const BlockMeta& bm);
private:
  const static size_t MAX_BLOCK_NUMB = sizeof(size_t) * 8;
  struct FileMeta {
    size_t file_numb_;
    size_t block_status_; // 1 valid 0 invalid
    size_t file_size_;
    size_t block_numb_;

    FileMeta(size_t a) {
      file_numb_ = a;
      block_status_ = 0;
      block_status_ = ~(block_status_);
      file_size_ = 0;
      block_numb_ = 0;
    }

    std::string Status() {
      std::stringstream ss;
      size_t status = block_status_;
      for (size_t i = 0; i < MAX_BLOCK_NUMB; ++ i) {
        if (status & 1)
          ss << 1;
        else
          ss << 0;
        status >>= 1;
      }
      return ss.str();
    }
  };
  size_t total_file_number_;
  std::vector<FileMeta> file_meta_;
  FileSystem *filesystem_;

  std::string GetFilename(size_t file_numb);

  size_t GetFileNumber();

  int FindFileMeta(size_t file_numb);

  // void CollectGarbage();

  BlockMeta WriteBlock(const std::string& block_data);

  void ShowFileMeta();
};
}

#endif