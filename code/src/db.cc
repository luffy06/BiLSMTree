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
  result_->lsmtreeresult_->Write(kv_.size());
  result_->lsmtreeresult_->RealWrite(kv_.size());
  std::vector<KV> kv = cacheserver_->Put(kv_);
  if (kv.size() > 0) {
    if (Config::TRACE_LOG)
      std::cout << "Ready to Minor Compaction" << std::endl;
    kvserver_->MinorCompact(kv);
    if (Config::TRACE_LOG)
      std::cout << "Minor Compaction Success" << std::endl;
  }
}

bool DB::Get(const std::string key, std::string& value) {
  result_->lsmtreeresult_->Read(key.size(), "MEMORY");
  result_->lsmtreeresult_->RealRead(key.size());
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
    result_->lsmtreeresult_->Read(value.size(), "MEMORY");
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