#ifndef BILSMTREE_CACHESERVER_H
#define BILSMTREE_CACHESERVER_H

namespace bilsmtree {

class Slice;
struct KV;
class SkipList;
class LRU2Q;

class CacheServerConfig {
public:
  CacheServer() { }
  ~CacheServer() { }

  const static size_t MAXSIZE = 10; // max size of immutable memtables
}

class CacheServer {
public:
  CacheServer();
  ~CacheServer();

  SkipList* Put(const KV& kv);

  Slice Get(const Slice& key);
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