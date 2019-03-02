#include "lru2q.h"

namespace bilsmtree {

LRU2Q::LRU2Q() {
  lru_ = new BiList(Config::LRU2QConfig::M1, Config::LRU2QConfig::M1_NUMB);
  fifo_ = new BiList(Config::LRU2QConfig::M2, Config::LRU2QConfig::M2_NUMB);
}

LRU2Q::~LRU2Q() {
  delete lru_;
  delete fifo_;
}

std::vector<KV> LRU2Q::Put(const KV kv) {
  // std::map<Slice, size_t>::iterator it = map_.find(kv.key_);
  std::vector<KV> res;
  // insert key into lru
  lru_->Insert(kv);
  std::vector<KV> lru_pop = lru_->Pop();
  // lru is full, append data to fifo
  for (size_t i = 0; i < lru_pop.size(); ++ i) {
    // fifo is full
    if (fifo_->IsFull())
      res = fifo_->DropAll();
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
      std::vector<KV> lru_pop = lru_->Pop();
      for (size_t i = 0; i < lru_pop.size(); ++ i) {
        assert(!fifo_->IsFull());
        fifo_->Append(lru_pop[i]);
      }
    }
  }
  return res;
}

}