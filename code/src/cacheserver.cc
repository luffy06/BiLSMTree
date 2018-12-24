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

std::vector<SkipList*> CacheServer::Put(const KV kv) {
  std::string algo = Util::GetAlgorithm();
  std::vector<SkipList*> res;
  if (algo == std::string("BiLSMTree")) {
    std::vector<KV> lru_pop = lru_->Put(kv);
    for (size_t i = 0; i < lru_pop.size(); ++ i) {
      KV pop_kv = lru_pop[i];
      // lru2q is full
      if (mem_->IsFull()) {
        // immutable memtable is full
        if (imms_.size() == Config::CacheServerConfig::MAXSIZE) {
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
          res.push_back(imms_[index].imm_);
          imms_.erase(imms_.begin() + index);
        }
        mem_->DisableWrite();
        // create a new listnode
        imms_.push_back(ListNode(mem_));
        // create a new immutable memtable
        mem_ = new SkipList();
      }
      // insert memtable
      mem_->Insert(pop_kv);
    }
  }
  else if (algo == std::string("LevelDB") || algo == std::string("Wisckey") || algo == std::string("BiLSMTree2") || algo == std::string("LevelDB-Sep")) {
    if (mem_->IsFull()) {
      if (imms_.size() != 0) {
        res.push_back(imms_[0].imm_);
        imms_.clear();
      }
      mem_->DisableWrite();
      imms_.push_back(ListNode(mem_));
      mem_ = new SkipList();
    }
    mem_->Insert(kv);
  }
  else {
    std::cout << "Algorithm Error" << std::endl;
    assert(false);
  }
  assert(res.size() < 2);
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