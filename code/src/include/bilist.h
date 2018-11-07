#ifndef BILSMTREE_BILIST_H
#define BILSMTREE_BILIST_H

#include <queue>

namespace bilsmtree {

class Slice;
struct KV;

class BiList {
public:
  BiList(size_t size);
  
  ~BiList();
  
  void Set(size_t pos, KV kv);

  void MoveToHead(size_t pos);

  KV Get(size_t pos);

  KV Delete(size_t pos);

  // insert after tail
  void Append(KV kv);

  // insert before head
  KV Insert(KV kv);

  size_t Head() { return data[head_].next_; }

  size_t Tail() { return tail_; }

private:
  struct ListNode {
    KV kv_;
    size_t next_, prev_;

    Node() { }

    Node(Slice& key, Slice& value) : kv_(key, value) { }
  };

  size_t head_;        // refer to the head of list which doesn't store data
  size_t tail_;        // refer to last data's position
  size_t max_size_;
  size_t data_size_;
  queue<size_t> free_;
  ListNode *data_;
};

}

#endif