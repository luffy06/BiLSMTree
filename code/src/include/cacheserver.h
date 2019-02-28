#ifndef BILSMTREE_CACHESERVER_H
#define BILSMTREE_CACHESERVER_H

#include "skiplist.h"
#include "lru2q.h"

namespace bilsmtree {

class CacheServer {
public:
  CacheServer(MemoryResult *memoryresult);

  ~CacheServer();

  std::vector<SkipList*> Put(const KV kv);

  bool Get(const Slice key, Slice& value);
private:
  struct ListNode {
    SkipList* imm_;
    size_t fre_;

    ListNode(SkipList* mem) {
      imm_ = mem;
      fre_ = 0;
    }
  };
  LRU2Q *lru_;
  SkipList *mem_;
  std::vector<ListNode> imms_;
  MemoryResult *memoryresult_;
  const std::vector<std::string> lru2q_algos = {"BiLSMTree"};
  const std::vector<std::string> base_algos = {"BiLSMTree-Direct", "Wisckey", "LevelDB", "Cuckoo"};
};
}

#endif