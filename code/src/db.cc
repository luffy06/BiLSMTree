#include "db.h"

namespace bilsmtree {

DB::DB() {
  result_ = new Result();
  filesystem_ = new FileSystem(result_->flashresult_);
  cacheserver_ = new CacheServer(result_->memoryresult_);
  kvserver_ = new KVServer(filesystem_, result_->lsmtreeresult_);
}

DB::~DB() {
  delete cacheserver_;
  delete kvserver_;
  delete filesystem_;
  delete result_;
}

void DB::Put(const std::string key, const std::string value) {
  KV kv_ = KV(key, value);
  result_->lsmtreeresult_->RealWrite(kv_.size());
  std::vector<SkipList*> skiplists = cacheserver_->Put(kv_);
  for (size_t i = 0; i < skiplists.size(); ++ i) {
    SkipList* sl = skiplists[i];
    // need compaction
    if (Config::TRACE_LOG)
      std::cout << "Ready to Minor Compaction" << std::endl;
    kvserver_->MinorCompact(sl);
    if (Config::TRACE_LOG)
      std::cout << "Minor Compaction Success" << std::endl;
    delete sl;
  }

}

bool DB::Get(const std::string key, std::string& value) {
  // std::cout << "Get" << std::endl;
  result_->lsmtreeresult_->RealRead(key_.size());
  Slice value_;
  Slice key_ = Slice(key.data(), key.size());
  bool res = cacheserver_->Get(key_, value_);
  if (!res) {
    res = kvserver_->Get(key_, value_);
    result_->lsmtreeresult_->ReadInFlash();
  }
  else {
    result_->lsmtreeresult_->ReadInMemory();
  }
  if (res) {
    result_->lsmtreeresult_->RealRead(value_.size());
    value = value_.ToString();
  }
  return res;
}

void DB::StartRecord() {
  filesystem_->StartRecord();
  result_->StartRecord();
}

}