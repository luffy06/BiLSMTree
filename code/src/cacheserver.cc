#include "cacheserver.h"

namespace bilsmtree {
CacheServer::CacheServer() {
  lru_ = new LRU2Q();
  imm_temp_ = new SkipList();
  head_ = new ListNode();
  head_->imm_ = NULL;
  head_->next_ = NULL;
  head_->prev_ = NULL;
  tail_ = NULL;
  imm_size_ = 0;
}

CacheServer::~CacheServer() {
  delete lru_;
  delete imm_temp_;
  for (ListNode* p = head_; head_ != NULL; p = head_) {
    head_ = head_->next_;
    delete p->imm_;
    delete p;
  }
}

// TODO
SkipList* CacheServer::Put(const KV kv) {
  KV kv_;
  SkipList* res = NULL;
  if (lru_->Put(kv, kv_)) {
    // lru2q is full
    if (imm_temp_->IsFull()) {
      // immutable memtable is full
      if (imm_size_ == Config::CacheServerConfig::MAXSIZE) {
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
      imm_temp_->DisableWrite();
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

bool CacheServer::Get(const Slice key, Slice& value) {
  if (!lru_->Get(key, value)) {
    bool imm_res = imm_temp_->Find(key, value);
    ListNode* p = head_->next_;
    while (p != NULL && !imm_res) {
      imm_res = p->imm_->Find(key, value);
      if (imm_res) {
        p->prev_->next_ = p->next_;
        if (p->next_ != NULL)
          p->next_->prev_ = p->prev_;
        p->next_ = head_->next_;
        p->prev_ = head_;
        if (head_->next_ != NULL)
          head_->next_->prev_ = p;
        head_->next_ = p;
      }
    }
    return imm_res;
  }
  return true;
}

}