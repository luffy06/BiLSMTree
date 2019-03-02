#include "cacheserver.h"

namespace bilsmtree {
CacheServer::CacheServer(MemoryResult *memoryresult) {
  lru_ = new LRU2Q();
  mem_ = new SkipList();
  imm_ = NULL;
  memoryresult_ = memoryresult;
}

CacheServer::~CacheServer() {
  delete lru_;
  delete mem_;
  if (imm_ != NULL)
    delete imm_;
}

std::vector<KV> CacheServer::Put(const KV kv) {
  std::string algo = Util::GetAlgorithm();
  std::vector<KV> res;
  if (Util::CheckAlgorithm(algo, lru2q_algos)) {
    res = lru_->Put(kv);
    sort(res.begin(), res.end(), [](KV a, KV b) {
      return a.key_.compare(b.key_) < 0;
    });
  }
  else if (Util::CheckAlgorithm(algo, base_algos)) {
    if (mem_->IsFull()) {
      if (imm_ != NULL) {
        res = imm_->GetAll();
        delete imm_;
      }
      imm_ = mem_;
      imm_->DisableWrite();
      mem_ = new SkipList();
    }
    mem_->Insert(kv);
  }
  else {
    std::cout << "Algorithm Error" << std::endl;
    assert(false);
  }
  return res;
}

bool CacheServer::Get(const Slice key, Slice& value) {
  bool res = false;
  std::string algo = Util::GetAlgorithm();
  if (Util::CheckAlgorithm(algo, lru2q_algos)) {
    res = lru_->Get(key, value);
  }
  else if (Util::CheckAlgorithm(algo, base_algos)) {
    res = mem_->Find(key, value);
    if (!res)
      res = imm_->Find(key, value);
  }
  memoryresult_->Hit((res ? 1 : 0));
  return res;
}
}