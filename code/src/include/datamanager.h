#ifndef BILSMTREE_DATAMANAGER_H
#define BILSMTREE_DATAMANAGER_H

namespace bilsmtree {

class DataManager {
public:
  DataManager();

  ~DataManager();
  
  BlockMeta Append(std::vector<KV> kvs);

  bool Get(const Slice key, Slice& value, size_t file_numb, size_t offset, size_t block_size);

  void Invalidate(BlockMeta bm);
private:
  struct FileMeta {
    size_t file_numb_;
    size_t block_status_;
    size_t file_size_;
    size_t block_numb_;
  };
  size_t total_file_number_;
  std::vector<FileMeta> file_meta_;
  FileSystem *filesystem_;

  std::string GetFilename(size_t file_numb);

  size_t GetFileNumber();

  int FindFileMeta(size_t file_numb);

  // void CollectGarbage();

  BlockMeta WriteBlock(std::string block_data);
};
}

#endif