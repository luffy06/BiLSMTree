#ifndef BILSMTREE_LOGMANAGER_H
#define BILSMTREE_LOGMANAGER_H

#include <string>

#include "slice.h"
#include "filesystem.h"

namespace bilsmtree {

class LogManager {
public:
  LogManager(FileSystem* filesystem);
  
  ~LogManager();

  Slice Append(const KV kv);

  Slice Get(const Slice location);

private:
  size_t head_;
  size_t tail_;
  size_t record_count_;
  FileSystem* filesystem_;

  size_t WriteKV(const KV kv);

  KV ReadKV(const size_t location, const size_t size_);
};

}

#endif