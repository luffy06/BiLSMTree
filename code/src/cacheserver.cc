#include "cacheserver.h"

namespace bilsmtree {
CacheServer::CacheServer(MemoryResult *memoryresult) {
  lru_ = new LRU2Q();
  mem_ = new SkipList();
  imm_ = NULL;
  imm_list_.clear();
  memoryresult_ = memoryresult;
  ShowMemory();
}

CacheServer::~CacheServer() {
  delete lru_;
  delete mem_;
  if (imm_ != NULL)
    delete imm_;
  for (size_t i = 0; i < imm_list_.size(); ++ i) 
    delete imm_list_[i];
}

std::vector<KV> CacheServer::Put(const KV kv) {
  std::string algo = Util::GetAlgorithm();
  std::vector<KV> res;
  if (Util::CheckAlgorithm(algo, lru2q_imm_algos)) {
    std::vector<KV> tmp = lru_->Put(kv, lru2q_algos, lru2q_imm_algos);
    for (size_t i = 0; i < tmp.size(); ++ i) {
      if (mem_->IsFull()) {
        if (imm_list_.size() == Config::CacheServerConfig::IMM_NUMB) {
          SkipList* pop = *(imm_list_.begin());
          imm_list_.erase(imm_list_.begin());
          std::vector<KV> buf = pop->GetAll();
          delete pop;
          for (size_t j = 0; j < buf.size(); ++ j)
            res.push_back(buf[j]);
        }
        imm_list_.push_back(mem_);
        mem_ = new SkipList();
      }
      mem_->Insert(tmp[i]);
    }
    sort(res.begin(), res.end(), [](KV a, KV b) {
      return a.key_.compare(b.key_) < 0;
    });    
  }
  else if (Util::CheckAlgorithm(algo, lru2q_algos)) {
    res = lru_->Put(kv, lru2q_algos, lru2q_imm_algos);
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
  if (Util::CheckAlgorithm(algo, lru2q_imm_algos)) {
    res = lru_->Get(key, value);
    if (!res) {
      res = mem_->Find(key, value);
      for (size_t i = 0; i < imm_list_.size() && !res; ++ i)
        res = imm_list_[i]->Find(key, value);
    }
  }
  if (Util::CheckAlgorithm(algo, lru2q_algos)) {
    res = lru_->Get(key, value);
  }
  else if (Util::CheckAlgorithm(algo, base_algos)) {
    res = mem_->Find(key, value);
    if (!res)
      res = imm_->Find(key, value);
  }
  else {
    std::cout << "Algorithm Error" << std::endl;
    assert(false);
  }
  memoryresult_->Hit((res ? 1 : 0));
  return res;
}

void CacheServer::ShowMemory() {
  size_t memory = Config::CacheServerConfig::MEMORY_SIZE;
  size_t lru2q_size_ = Util::GetLRU2QSize();
  size_t mem_size_ = Util::GetMemTableSize();
  size_t m1 = lru2q_size_ * Config::CacheServerConfig::LRU_RATE / (Config::CacheServerConfig::LRU_RATE + 1);
  size_t m2 = lru2q_size_ - m1;
  std::cout << "MEMORY:" << memory << std::endl;
  std::cout << "LRU:" << m1 << "\tFIFO:" << m2 << std::endl;
  std::cout << "MEM:" << mem_size_ << std::endl;
  std::cout << "IMM_NUMB:" << Util::GetMemTableNumb() << std::endl;
}
}