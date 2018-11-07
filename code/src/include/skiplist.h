#ifndef BILSMTREE_SKIPLIST_H
#define BILSMTREE_SKIPLIST_H

#include <vector>

namespace bilsmtree {

class Slice;
struct KV;

class ImmutableMemTableConfig {
public:
  ImmutableMemTableConfig() { }
  ~ImmutableMemTableConfig() { }

  const static size_t MAXSIZE = 4096;
};

class SkipList {
public:
  SkipList();

  ~SkipList();

  bool IsFull();

  Slice Find(const Slice& key);
  
  void Insert(const KV& kv);

  void Delete(const Slice& key);

  std::vector<KV> GetAll();
private:
  struct ListNode {
    KV kv_;
    size_t level_;
    ListNode **forward_;
  };

  const double PROB = 0.5;
  const size_t MAXLEVEL = 4096;
  size_t data_size_;
  ListNode *head_;

  size_t GenerateLevel();
};

}

#endif