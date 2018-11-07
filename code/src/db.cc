#include "db.h"

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
  SkipList* sl = cacheserver_->Put(kv);
  if (sl != NULL) {
    // need compaction
    kvserver_->Compact(sl);
  }
}

Slice DB::Get(const Slice& key) {
  Slice value = cacheserver_->Get(key);
  if (value == NULL)
    value = kvserver_->Get(key);
  return value;
}

}