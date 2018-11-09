#ifndef BILSMTREE_LOGMANAGER_H
#define BILSMTREE_LOGMANAGER_H

#include <string>

#include "slice.h"
#include "filesystem.h"

namespace bilsmtree {

class Slice;
struct KV;
class FileSystem;

class LogManager {
public:
  LogManager();
  ~LogManager();

  Slice Append(const KV kv);

  Slice Get(const Slice location);

private:
  uint head_;
  uint tail_;
  uint record_count_;

  void WriteKV(const KV kv);

  KV ReadKV(const uint location);
};

}

#endif