#ifndef BILSMTREE_KVSERVER_H
#define BILSMTREE_KVSERVER_H

#include <vector>

#include "lsmtree.h"
#include "logmanager.h"
#include "skiplist.h"

namespace bilsmtree {

class SkipList;
class LSMTree;
class LogManager;
class FileSystem;

class KVServer {
public:
  KVServer(FileSystem* filesystem);
  
  ~KVServer();

  bool Get(const Slice key, Slice& value);

  void MinorCompact(const SkipList* sl);
private:
  LSMTree *lsmtree_;
  LogManager *logmanager_;
};

}

#endif