namespace bilsmtree {
DB::DB() {
  cacheserver_ = new CacheServer();
  kvserver_ = new KVServer();
}

DB::~DB() {
  delete cacheserver_;
  delete kvserver_;
}

void DB::Put(const Slice& key, const Slice& value) {
  KV kv_ = KV(key, value);
  SkipList* res = cacheserver_->Put(kv);
  if (res != NULL) {
    // need compaction
    kvserver_->Compact(res);
  }
}

Slice DB::Get(const Slice& key) {
  Slice value = cacheserver_->Get(key);
  if (value == NULL)
    value = kvserver_->Get(key);
  return value;
}

}