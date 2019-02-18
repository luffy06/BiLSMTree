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

  size_t GetAllSize() const {
    size_t size_ = 0;
    ListNode *p = head_;
    for (size_t i = 0; i < data_numb_; ++ i) {
      KV kv = p->forward_[0]->kv_;
      size_ = size_ + kv.size();
      p = p->forward_[0];
    }
    return size_;
  }

  std::vector<KV> GetAll() const {
    std::vector<KV> kvs;
    ListNode *p = head_;
    for (size_t i = 0; i < data_numb_; ++ i) {
      KV kv = p->forward_[0]->kv_;
      kvs.push_back(kv);
      p = p->forward_[0];
    }
    return kvs;
  }

  void DisableWrite();

  size_t data_size() { return data_size_; }
private:
  struct ListNode {
    KV kv_;
    size_t level_;
    ListNode **forward_;

    ~ListNode() { delete[] forward_; }
  };

  size_t data_size_;
  size_t data_numb_;
  ListNode *head_;
  bool writable_;

  size_t GenerateLevel();
};

}

#endif