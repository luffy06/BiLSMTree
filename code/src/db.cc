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

void DB::Put(const std::string key, const std::string value) {
  KV kv_ = KV(key, value);
  SkipList* sl = cacheserver_->Put(kv_);
  if (sl != NULL) {
    // need compaction
    kvserver_->MinorCompact(sl);
  }
}

bool DB::Get(const std::string key, std::string& value) {
  Slice value_;
  bool res = false;
  if (!cacheserver_->Get(key, value_))
    res = kvserver_->Get(key, value_);
  if (res)
    value = value_.ToString();
  return res;
}

}