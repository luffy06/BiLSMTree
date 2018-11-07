#include "cacheserver.h"

namespace bilsmtree {
CacherServer::CacherServer() {
  lru_ = new LRU2Q();
  imm_temp_ = new SkipList();
  head_ = new ListNode();
  head_->imm_ = NULL;
  head_->next_ = NULL;
  head_->prev_ = NULL;
  tail_ = NULL;
  imm_size_ = 0;
}

CacherServer::~CacherServer() {
  delete lru_;
  delete imm_temp_;
  for (ListNode* p = head_; head_ != NULL; p = head_) {
    head_ = head_->next_;
    delete p->imm_;
    delete p;
  }
}

// TODO
SkipList* Put(const KV& kv) {
  KV kv_ = lru_->Put(kv);
  SkipList* res = NULL;
  if (kv_ != NULL) {
    // lru2q is full
    if (imm_temp_->IsFull()) {
      // immutable memtable is full
      if (imm_size_ == CacherServerConfig::MAXSIZE) {
        // the size of immmutable memtables is full
        // record tail position
        ListNode* p = tail_;
        // move tail pointer
        tail_ = tail_->prev_;
        tail_->next_ = NULL;
        // record immutable memtable which is ready to dump
        res = p->imm_;
        // delete tail node
        delete p;
        imm_size_ = imm_size_ - 1;
      }
      // create a new listnode
      ListNode* p = new ListNode();
      p->imm_ = imm_temp_;
      p->prev_ = head_;
      p->next_ = head_->next_;
      if (head_->next_ != NULL)
        head_->next_->prev_ = p;
      head_->next_ = p;
      imm_size_ = imm_size_ + 1;
      // create a new immutable memtable
      imm_temp_ = new SkipList();
    }
    imm_temp_->Insert(kv);
  }
  return res;
}

Slice Get(const Slice& key) {
  Slice value = lru_->Get(key);
  if (value == NULL)
    value = imm_temp_->Find(key);
  ListNode* p = head_->next_;
  while (value == NULL && p != NULL)
    value = p->imm_->Find(key);
  return value;
}

}