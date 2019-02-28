#ifndef BILSMTREE_KVSERVER_H
#define BILSMTREE_KVSERVER_H

#include <vector>

#include "lsmtree.h"
#include "logmanager.h"
#include "skiplist.h"

namespace bilsmtree {

class KVServer {
public:
  KVServer(FileSystem* filesystem, LSMTreeResult* lsmtreeresult);
  
  ~KVServer();

  bool Get(const Slice key, Slice& value);

  void MinorCompact(const SkipList* sl);
private:
  LSMTree *lsmtree_;
  LogManager *logmanager_;
  const std::vector<std::string> kvsep_algos = {"BiLSMTree-Direct", "Wisckey", "BiLSMTree", "Cuckoo"};
  const std::vector<std::string> base_algos = {"LevelDB"};
};

}

#endif