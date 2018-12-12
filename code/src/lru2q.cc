#include "lru2q.h"

namespace bilsmtree {

LRU2Q::LRU2Q() {
  map_.clear();
  lru_ = new BiList(Config::LRU2QConfig::M1, Config::LRU2QConfig::M1_NUMB);
  fifo_ = new BiList(Config::LRU2QConfig::M2, Config::LRU2QConfig::M2_NUMB);
}

LRU2Q::~LRU2Q() {
  map_.clear();
  delete lru_;
  delete fifo_;
}

/*
* STATUS: checking
*/
bool LRU2Q::Put(const KV kv, KV& pop_kv) {
  // std::map<Slice, size_t>::iterator it = map_.find(kv.key_);
  std::pair<size_t, int> pos = GetPos(kv.key_);
  bool res = false;
  if (pos.second != -1) {
    // lru has the key
    size_t index = pos.second;
    if (index <= Config::LRU2QConfig::M1) {
      lru_->Set(index, kv);
    }
    else {
      fifo_->Set(index - Config::LRU2QConfig::M1, kv);
    }
    MoveToHead(index);
    // update map of data
    map_[pos.first] = std::make_pair(kv.key_, lru_->Head());
  }
  else {
    // lru doesn't has the key
    // insert key into lru
    KV p_kv;
    bool lru_res = lru_->Insert(kv, p_kv);
    // lru is full
    if (lru_res) { // uncheck
      // append data to fifo
      res = fifo_->Append(p_kv, pop_kv);
      // fifo is full
      if (res) {
        std::pair<size_t, int> pp_pos = GetPos(pop_kv.key_);
        map_.erase(map_.begin() + pp_pos.first);
      }
      std::pair<size_t, int> p_pos = GetPos(p_kv.key_);
      map_[p_pos.first] = std::make_pair(p_kv.key_, fifo_->Tail() + Config::LRU2QConfig::M1);
    }
    // update map of data
    map_.push_back(std::make_pair(kv.key_, lru_->Head()));
  }
  return res;
}

bool LRU2Q::Get(const Slice key, Slice& value) {
  // std::vector<KV> res = GetAll();
  // std::cout << std::string(20, '#') << std::endl;
  // for (size_t i = 0; i < res.size(); ++ i) {
  //   std::cout << res[i].key_.ToString() << std::endl;
  // }
  // std::cout << std::string(20, '#') << std::endl;
  std::pair<size_t, int> pos = GetPos(key);
  // std::cout << pos.first << "\t" << pos.second << std::endl;
  if (pos.second != -1) {
    size_t index = pos.second;
    if (index <= Config::LRU2QConfig::M1)
      value = lru_->Get(index).value_;
    else
      value = fifo_->Get(index - Config::LRU2QConfig::M1).value_;
    MoveToHead(index);
    // update map of data
    map_[pos.first] = std::make_pair(key, lru_->Head());
    return true;
  }
  return false;
}

std::pair<size_t, int> LRU2Q::GetPos(const Slice key) {
  for (size_t i = 0; i < map_.size(); ++ i) {
    if (map_[i].first.compare(key) == 0)
      return std::make_pair(i, map_[i].second);
  }
  return std::make_pair(0, -1);
}

void LRU2Q::MoveToHead(size_t index) {
  if (index <= Config::LRU2QConfig::M1) {
    // in lru queue
    // move to the head of lru
    lru_->MoveToHead(index);
  }
  else {
    // in fifo queue
    index = index - Config::LRU2QConfig::M1;
    // delete data of fifo
    KV ln = fifo_->Delete(index);
    // copy data of the tail of lru 
    size_t lru_tail_ = lru_->Tail();
    KV ln_tail = lru_->Get(lru_tail_);
    // set data to the tail of lru and move to the head of lru
    lru_->Set(lru_tail_, ln);
    lru_->MoveToHead(lru_tail_);
    // append the original tail of lru to fifo
    KV p_kv; 
    assert(!fifo_->Append(ln_tail, p_kv));
    std::pair<size_t, int> pos = GetPos(ln_tail.key_);
    map_[pos.first] = std::make_pair(ln_tail.key_, fifo_->Tail() + Config::LRU2QConfig::M1);
  }
}

}