#include <vector>

namespace bilsmtree {

class CacheServerConfig {
public:
  CacheServer() { }
  ~CacheServer() { }

  const static int MAXSIZE = 10; // max size of immutable memtables
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
  LRU2Q* lru_;
  SkipList* imm_temp_;
  int imm_size_;
  ListNode* head_;
  ListNode* tail_;
};
}