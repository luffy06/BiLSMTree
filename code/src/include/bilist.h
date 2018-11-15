#ifndef BILSMTREE_BILIST_H
#define BILSMTREE_BILIST_H

#include <queue>

#include "slice.h"
#include "util.h"

namespace bilsmtree {

/*
* STAUTS: SUCCESS
*/
class BiList {
public:
  BiList(size_t size);
  
  ~BiList();
  
  void Set(size_t pos, const KV kv);

  void MoveToHead(size_t pos);

  KV Get(size_t pos);

  KV Delete(size_t pos);

  // insert after tail, pop head
  // true: has key pop
  // false: no key pop
  bool Append(const KV kv, KV& pop_kv);

  // insert before head, pop tail
  // true: has key pop
  // false: no key pop  
  bool Insert(const KV kv, KV& pop_kv);

  size_t Head() { return data_[head_].next_; }

  size_t Tail() {
    assert(tail_ > 0);
    return tail_;
  }

  std::vector<KV> GetAll() {
    std::vector<KV> res;
    size_t p = data_[head_].next_;
    while (p) {
      res.push_back(data_[p].kv_);
      p = data_[p].next_;
    }
    return res;
  }

  void Show() {
    size_t p = data_[head_].next_;
    size_t i = 1;
    while (p) {
      std::cout << "Num: " << i++ << " Index: " << p << std::endl;
      std::cout << "Key:" << data_[p].kv_.key_.ToString() << std::endl;
      std::cout << "value:" << data_[p].kv_.value_.ToString() << std::endl;
      p = data_[p].next_;
    }
  }

private:
  struct ListNode {
    KV kv_;
    size_t next_, prev_;

    ListNode() { }

    ListNode(const Slice key, const Slice value) : kv_(key, value) { }
  };

  size_t head_;        // refer to the head of list which doesn't store data
  size_t tail_;        // refer to last data's position
  size_t max_size_;
  size_t data_size_;
  std::queue<size_t> free_;
  ListNode *data_;
};

}

#endif