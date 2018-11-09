#ifndef BILSMTREE_CACHESERVER_H
#define BILSMTREE_CACHESERVER_H

#include "skiplist.h"
#include "lru2q.h"

namespace bilsmtree {

class SkipList;
class LRU2Q;

class CacheServer {
public:
  CacheServer();
  ~CacheServer();

  SkipList* Put(const KV kv);

  bool Get(const Slice key, Slice& value);
private:
  struct ListNode {
    SkipList* imm_;
    ListNode* next_;
    ListNode* prev_;
  };
  LRU2Q *lru_;
  SkipList *imm_temp_;
  size_t imm_size_;
  ListNode *head_;
  ListNode *tail_;
};
}

#endif