#include "cacheserver.h"

namespace bilsmtree {
CacheServer::CacheServer() {
  lru_ = new LRU2Q();
  mem_ = new SkipList();
  head_ = new ListNode();
  head_->imm_ = NULL;
  head_->next_ = NULL;
  head_->prev_ = NULL;
  tail_ = NULL;
  imm_size_ = 0;
}

CacheServer::~CacheServer() {
  delete lru_;
  delete mem_;
  for (ListNode* p = head_; head_ != NULL; p = head_) {
    head_ = head_->next_;
    delete p->imm_;
    delete p;
  }
}

SkipList* CacheServer::Put(const KV kv) {
  KV kv_;
  SkipList* res = NULL;
  if (lru_->Put(kv, kv_)) {
    // std::cout << "POP KV" << std::endl;
    // kv_.show();
    // lru2q is full
    if (mem_->IsFull()) {
      // std::cout << "Memtable is full" << std::endl;
      // immutable memtable is full
      if (imm_size_ == Config::CacheServerConfig::MAXSIZE) {
        // the size of immutable memtables is full
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
      mem_->DisableWrite();
      // create a new listnode
      ListNode* p = new ListNode();
      p->imm_ = mem_;
      p->prev_ = head_;
      p->next_ = head_->next_;
      if (head_->next_ != NULL)
        head_->next_->prev_ = p;
      head_->next_ = p;
      imm_size_ = imm_size_ + 1;
      // create a new immutable memtable
      mem_ = new SkipList();
    }
    // insert memtable
    mem_->Insert(kv);
  }
  return res;
}

bool CacheServer::Get(const Slice key, Slice& value) {
  std::cout << "Get In CacheServer" << std::endl;
  if (!lru_->Get(key, value)) {
    std::cout << "Get In Memetable" << std::endl;
    bool imm_res = mem_->Find(key, value);
    ListNode *p = head_->next_;
    size_t j = 0;
    std::cout << "Get In Immutable Memetable" << std::endl;
    while (p != NULL && !imm_res) {
      std::cout << "Get In Immutable Memetable:" << j << std::endl;
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
        break;
      }
      else {
        std::cout << "MEMTABLE " << j << " DOESN't FIND" << std::endl;
        std::vector<KV> res = p->imm_->GetAll();
        std::cout << res.size() << std::endl;
        for (size_t i = 0; i < res.size(); ++ i)
          std::cout << res[i].key_.ToString() << std::endl;
      }
      ++ j;
      p = p->next_;
    }
    return imm_res;
  }
  return true;
}

}