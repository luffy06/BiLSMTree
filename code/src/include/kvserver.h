#ifndef BILSMTREE_KVSERVER_H
#define BILSMTREE_KVSERVER_H

#include <vector>

namespace bilsmtree {

class Slice;
struct KV;
class SkipList;
class LSMTree;
class LogManager;

class KVServer {
public:
  KVServer();
  
  ~KVServer();

  Slice Get(const Slice& value);

  void MinorCompact(const SkipList* sl);
private:
  LSMTree *lsmtree_;
  LogManager *logmanager_;
};

}

#endif