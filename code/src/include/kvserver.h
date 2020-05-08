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

  void Splay(const double read_ratio);

  bool NeedRollback();

  std::vector<KV> GetRollbackData();
private:
  LSMTree *lsmtree_;
  LogManager *logmanager_;
  const std::vector<std::string> kvsep_algos = {"BiLSMTree", "BiLSMTree-Ext", "BiLSMTree-Direct", "BiLSMTree-Mem", "Wisckey", "Cuckoo"};
  const std::vector<std::string> base_algos = {"BiLSMTree-KV", "LevelDB"};
  const std::vector<std::string> rollback_mem_algos = {"BiLSMTree-Mem"};
};

}

#endif