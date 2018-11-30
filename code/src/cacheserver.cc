#include "cacheserver.h"

namespace bilsmtree {
CacheServer::CacheServer() {
  lru_ = new LRU2Q();
  mem_ = new SkipList();
  imms_.clear();
}

CacheServer::~CacheServer() {
  delete lru_;
  delete mem_;
  for (size_t i = 0; i < imms_.size(); ++ i)
    delete imms_[i].imm_;
}

SkipList* CacheServer::Put(const KV kv) {
  KV pop_kv;
  SkipList* res = NULL;
  if (lru_->Put(kv, pop_kv)) {
    // std::cout << "POP KV:" << pop_kv.key_.ToString() << std::endl;
    // lru2q is full
    if (mem_->IsFull()) {
      // std::cout << "Memtable is full Immutable Memtable Size:" << imm_size_ << std::endl;
      // immutable memtable is full
      if (imms_.size() == Config::CacheServerConfig::MAXSIZE) {
        // std::cout << "DUMP" << std::endl;
        // the size of immutable memtables is full
        // record tail position
        size_t min_fre = imms_[0].fre_;
        size_t index = 0;
        for (size_t i = 1; i < imms_.size(); ++ i) {
          if (imms_[i].fre_ < min_fre) {
            min_fre = imms_[i].fre_;
            index = i;
          }
        }
        res = imms_[index].imm_;
        imms_.erase(imms_.begin() + index);
      }
      mem_->DisableWrite();
      // create a new listnode
      ListNode p = ListNode(mem_);
      imms_.push_back(p);
      // create a new immutable memtable
      mem_ = new SkipList();
    }
    // std::cout << "Insert in Mem" << std::endl;
    // insert memtable
    mem_->Insert(pop_kv);
    // std::cout << std::string(10, '@') << std::endl;
  }
  return res;
}

bool CacheServer::Get(const Slice key, Slice& value) {
  // std::cout << "Find In CacheServer Immutable List Size:" << imms_.size() << std::endl;
  // std::cout << "Ready Find in LRU2Q" << std::endl;
  if (!lru_->Get(key, value)) {
    // std::cout << "Ready Find in Memtable" << std::endl;
    bool imm_res = mem_->Find(key, value);
    if (!imm_res) {
      for (size_t i = 0; i < imms_.size(); ++ i) {
        imm_res = imms_[i].imm_->Find(key, value);
        if (imm_res) {
          imms_[i].fre_ = imms_[i].fre_ + 1;
          break;
        }
      }
    }
    return imm_res;
  }
  return true;
}

}