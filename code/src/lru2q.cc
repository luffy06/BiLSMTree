#include "lru2q.h"

namespace bilsmtree {

LRU2Q::LRU2Q() {
  map_.clear();
  lru_ = new BiList(LRU2QConfig::M1);
  fifo_ = new BiList(LRU2QConfig::M2);
}

LRU2Q::~LRU2Q() {
  map_.clear();
  delete lru_;
  delete fifo_;
}

KV LRU2Q::Put(const KV& kv) {
  auto it = map_.find(kv.key_);
  KV res = NULL;
  if (it != map_.end()) {
    // lru has the key
    size_t index = *it;
    if (index < LRU2QConfig::M1) {
      // in lru queue
      // update value
      lru_.Set(index, kv);
      // move to the head of lru
      lru_.MoveToHead(index);
    }
    else {
      // in fifo queue
      index = index - LRU2QConfig::M1;
      // delete data of fifo
      KV ln = fifo_.Delete(index);
      // copy data of the tail of lru 
      size_t lru_tail_ = lru_.Tail();
      KV ln_tail = lru_.Get(lru_tail_);
      // set data to the tail of lru and move to the head of lru
      lru_.Set(lru_tail_, ln);
      lru_.MoveToHead(lru_tail_);
      // append the original tail of lru to fifo
      KV out = fifo_.Append(ln_tail);
      Util::Assert("FIFO queue is full!\nMethod: LRU2Q::Put", (out == NULL));
    }
    // update map of data
    map_[kv.key_] = lru_.Head();
  }
  else {
    // lru doesn't has the key
    // insert key into lru
    KV ln = lru_.Insert(kv);
    // lru is full
    if (ln != NULL) {
      // append data to fifo
      KV out = fifo_.Append(ln);

      // fifo is full
      if (out != NULL) {
        res = out;
        map_.earse(out.key_);
      }
      map_[ln.key_] = fifo.Tail();
    }
    // update map of data
    map_[kv.key_] = lru_.Head();
  }
  return res;
}

Slice LRU2Q::Get(const Slice& key) {
  auto it = map_[key];
  if (it != map_.end()) {
    size_t index = *it;
    if (index < LRU2QConfig::M1)
      return lru_.Get(index).value_;
    else
      return fifo_.Get(index - LRU2QConfig::M1).value_;
  }
  return NULL;
}

}