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

  bool IsFull();
  
  // insert after tail, pop head
  // true: has key pop
  // false: no key pop
  int Append(const KV kv);

  // insert before head, pop tail
  // true: has key pop
  // false: no key pop  
  int Insert(const KV kv);

  std::vector<KV> PopTail();

  std::vector<KV> PopHead();

  std::vector<KV> DropAll();

  KV Delete(int pos);

  KV DropHead();

  int GetSize() { return data_numb_; }

  bool Get(const Slice key, Slice& value);

  std::vector<KV> GetAll() {
    std::vector<KV> res;
    int p = data_[head_].next_;
    while (p != -1) {
      res.push_back(data_[p].kv_);
      p = data_[p].next_;
    }
    return res;
  }

  void Show() {
    int p = data_[head_].next_;
    size_t i = 1;
    while (p != -1) {
      std::cout << "Num: " << i++ << " Index: " << p << std::endl;
      std::cout << "Key:" << data_[p].kv_.key_.ToString() << std::endl;
      std::cout << "value:" << data_[p].kv_.value_.ToString() << std::endl;
      p = data_[p].next_;
    }
  }

private:
  struct ListNode {
    KV kv_;
    int next_, prev_;

    ListNode() { }

    ListNode(const Slice key, const Slice value) : kv_(key, value) { }
  };

  int head_;        // refer to the head of list which doesn't store data
  int tail_;        // refer to last data's position
  size_t max_size_;
  int data_size_;
  size_t max_numb_;
  int data_numb_;
  std::queue<size_t> free_;
  std::vector<ListNode> data_;

  void Init(size_t size, size_t numb);

  int ExistAndUpdate(const KV kv);
};

}

#endif