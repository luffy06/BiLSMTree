#include "db.h"

namespace bilsmtree {

DB::DB() {
  result_ = new Result();
  filesystem_ = new FileSystem(result_->flashresult_);
  cacheserver_ = new CacheServer();
  kvserver_ = new KVServer(filesystem_, result_->lsmtreeresult_);
}

DB::~DB() {
  delete cacheserver_;
  delete kvserver_;
  delete filesystem_;
}

void DB::Put(const std::string key, const std::string value) {
  KV kv_ = KV(key, value);
  SkipList* sl = cacheserver_->Put(kv_);
  if (sl != NULL) {
    // need compaction
    // std::cout << "Read to Compaction" << std::endl;
    kvserver_->MinorCompact(sl);
  }
}

bool DB::Get(const std::string key, std::string& value) {
  Slice value_;
  bool res = cacheserver_->Get(key, value_);
  if (!res) {
    res = kvserver_->Get(key, value_);
  }
  if (res)
    value = value_.ToString();
  return res;
}

}