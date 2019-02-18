#ifndef BILSMTREE_LOGMANAGER_H
#define BILSMTREE_LOGMANAGER_H

#include <string>

#include "slice.h"
#include "filesystem.h"

namespace bilsmtree {

class LogManager {
public:
  LogManager(FileSystem* filesystem, LSMTreeResult *lsmtreeresult);
  
  ~LogManager();

  std::vector<KV> Append(const std::vector<KV> kvs);

  Slice Get(const Slice location);

private:
  size_t file_size_;
  size_t record_count_;
  FileSystem* filesystem_;
  std::string buffer_;
  LSMTreeResult *lsmtreeresult_;

  KV ReadKV(const size_t location, const size_t size_);
};

}

#endif