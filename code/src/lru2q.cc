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

/*
* STATUS: checking
*/
std::vector<KV> LRU2Q::Put(const KV kv) {
  // std::map<Slice, size_t>::iterator it = map_.find(kv.key_);
  std::vector<KV> res;
  // insert key into lru
  std::vector<KV> lru_pop;
  lru_pop = lru_->Insert(kv);
  // lru is full
  if (lru_pop.size() > 0) {
    // append data to fifo
    for (size_t i = 0; i < lru_pop.size(); ++ i) {
      std::vector<KV> fifo_pop = fifo_->Append(lru_pop[i]);
      // fifo is full
      for (size_t j = 0; j < fifo_pop.size(); ++ j)
        res.push_back(fifo_pop[i]);
    }
  }
  return res;
}

bool LRU2Q::Get(const Slice key, Slice& value) {
  if (!lru_->Get(key, value))
    return fifo_->Get(key, value);
  return true;
}

}