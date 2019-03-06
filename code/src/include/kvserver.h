#ifndef BILSMTREE_KVSERVER_H
#define BILSMTREE_KVSERVER_H

#include <vector>

#include "lsmtree.h"
#include "logmanager.h"

namespace bilsmtree {

class KVServer {
public:
  KVServer(FileSystem* filesystem, LSMTreeResult* lsmtreeresult);
  
  ~KVServer();

  bool Get(const Slice key, Slice& value);

  void MinorCompact(const std::vector<KV> &data);
private:
  LSMTree *lsmtree_;
  LogManager *logmanager_;
  const std::vector<std::string> kvsep_algos = {"BiLSMTree-Direct", "LevelDB-Sep", "Wisckey", "BiLSMTree", "Cuckoo"};
  const std::vector<std::string> base_algos = {"LevelDB"};
};

}

#endif