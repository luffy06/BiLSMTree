#ifndef BILSMTREE_CACHESERVER_H
#define BILSMTREE_CACHESERVER_H

#include "skiplist.h"
#include "lru2q.h"

namespace bilsmtree {

class CacheServer {
public:
  CacheServer(MemoryResult *memoryresult);

  ~CacheServer();

  std::vector<KV> Put(const KV kv);

  bool Get(const Slice key, Slice& value);
private:
  LRU2Q *lru_;
  SkipList *mem_;
  std::vector<SkipList*> imm_list_;
  SkipList *imm_;
  MemoryResult *memoryresult_;
  const std::vector<std::string> lru2q_imm_algos = {"BiLSMTree"};
  const std::vector<std::string> lru2q_algos = {"BiLSMTree-Ext", "BiLSMTree-Direct"};
  const std::vector<std::string> base_algos = {"Wisckey", "LevelDB", "Cuckoo"};

  void ShowMemory();
};
}

#endif