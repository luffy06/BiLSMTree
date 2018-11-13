#ifndef BILSMTREE_SKIPLIST_H
#define BILSMTREE_SKIPLIST_H

#include <vector>

#include "slice.h"
#include "util.h"

namespace bilsmtree {

class SkipList {
public:
  SkipList();

  ~SkipList();

  bool IsFull();

  bool Find(const Slice key, Slice& value);
  
  void Insert(const KV kv);

  void Delete(const Slice key);

  std::vector<KV> GetAll() const;

  void DisableWrite();
private:
  struct ListNode {
    KV kv_;
    size_t level_;
    ListNode **forward_;
  };

  size_t data_size_;
  ListNode *head_;
  bool writable_;

  size_t GenerateLevel();
};

}

#endif