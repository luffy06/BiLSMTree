#ifndef BILSMTREE_BILIST_H
#define BILSMTREE_BILIST_H

#include <queue>

#include "slice.h"
#include "util.h"

namespace bilsmtree {

class Slice;
struct KV;
class Util;

class BiList {
public:
  BiList(uint size);
  
  ~BiList();
  
  void Set(uint pos, const KV kv);

  void MoveToHead(uint pos);

  KV Get(uint pos);

  KV Delete(uint pos);

  // insert after tail
  // true: has key pop
  // false: no key pop
  bool Append(const KV kv, KV& pop_kv);

  // insert before head
  // true: has key pop
  // false: no key pop  
  bool Insert(const KV kv, KV& pop_kv);

  uint Head() { return data_[head_].next_; }

  uint Tail() { return tail_; }

private:
  struct ListNode {
    KV kv_;
    uint next_, prev_;

    ListNode() { }

    ListNode(const Slice key, const Slice value) : kv_(key, value) { }
  };

  uint head_;        // refer to the head of list which doesn't store data
  uint tail_;        // refer to last data's position
  uint max_size_;
  uint data_size_;
  std::queue<uint> free_;
  ListNode *data_;
};

}

#endif