#include "lru2q.h"

namespace bilsmtree {

LRU2Q::LRU2Q() {
  std::string algo = Util::GetAlgorithm();
  size_t lru2q_size_ = Util::GetLRU2QSize();
  size_t m1 = lru2q_size_ * Config::CacheServerConfig::LRU_RATE / (Config::CacheServerConfig::LRU_RATE + 1);
  size_t m2 = lru2q_size_ - m1;
  lru_ = new BiList(m1);
  fifo_ = new BiList(m2);
}

LRU2Q::~LRU2Q() {
  delete lru_;
  delete fifo_;
}

std::vector<KV> LRU2Q::Put(const KV kv, const std::vector<std::string> &lru2q_algos, const std::vector<std::string> &lru2q_imm_algos) {
  std::string algo = Util::GetAlgorithm();
  std::vector<KV> res;
  // insert key into lru
  lru_->Insert(kv);
  std::vector<KV> lru_pop = lru_->PopTail();
  // lru is full, append data to fifo
  for (size_t i = 0; i < lru_pop.size(); ++ i) {
    // fifo is full
    if (fifo_->IsFull()) {
      if (Util::CheckAlgorithm(algo, lru2q_algos)) {
        std::vector<KV> tmp = fifo_->DropAll();
        for (size_t j = 0; j < tmp.size(); ++ j)
          res.push_back(tmp[j]);
      }
      else if (Util::CheckAlgorithm(algo, lru2q_imm_algos)) {
        std::vector<KV> tmp = fifo_->PopTail();
        for (size_t j = 0; j < tmp.size(); ++ j)
          res.push_back(tmp[j]);
      }
      else {
        std::cout << "Algorithm Error" << std::endl;
        assert(false);
      }
    }
    fifo_->Append(lru_pop[i]);
  }
  return res;
}

bool LRU2Q::Get(const Slice key, Slice& value) {
  bool res = lru_->Get(key, value);
  if (!res) {
    res = fifo_->Get(key, value);
    if (res) {
      KV kv = fifo_->DropHead();
      lru_->Insert(kv);
      std::vector<KV> lru_pop = lru_->PopTail();
      for (size_t i = 0; i < lru_pop.size(); ++ i) {
        assert(!fifo_->IsFull());
        fifo_->Append(lru_pop[i]);
      }
    }
  }
  return res;
}

}