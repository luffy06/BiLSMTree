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

KV LRU2Q::Put(const Slice& key, const Slice& value) {
  auto it = map_.find(key);
  KV kv = NULL;
  if (it != map_.end()) {
    // lru has the key
    int index = *it;
    if (index < LRU2QConfig::M1) {
      // in lru queue
      // update value
      lru_.Set(index, KV(key, value));
      // move to the head of lru
      lru_.MoveToHead(index);
    }
    else {
      // in fifo queue
      index = index - LRU2QConfig::M1;
      // delete data of fifo
      KV ln = fifo_.Delete(index);
      // copy data of the tail of lru 
      int lru_tail_ = lru_.Tail();
      KV ln_tail = lru_.Get(lru_tail_);
      // set data to the tail of lru and move to the head of lru
      lru_.Set(lru_tail_, ln);
      lru_.MoveToHead(lru_tail_);
      // append the original tail of lru to fifo
      KV out = fifo_.Append(ln_tail);
      Util::Assert("FIFO queue is full!\nMethod: LRU2Q::Put", (out == NULL));
    }
    // update map of data
    map_[key] = lru_.Head();
  }
  else {
    // lru doesn't has the key
    // insert key into lru
    KV ln = lru_.Insert(KV(key, value));
    // lru is full
    if (ln != NULL) {
      // append data to fifo
      KV out = fifo_.Append(ln);
      // fifo is full
      if (out != NULL)
        kv = out;
    }
    // update map of data
    map_[key] = lru_.Head();
    // delete map
    map_.erase(kv.key_);
  }
  return kv;
}

Slice LRU2Q::Get(const Slice& key) {
  auto it = map_[key];
  if (it != map_.end()) {
    int index = *it;
    if (index < LRU2QConfig::M1)
      return lru_.Get(index).value_;
    else
      return fifo_.Get(index - LRU2QConfig::M1).value_;
  }
  return NULL;
}

}