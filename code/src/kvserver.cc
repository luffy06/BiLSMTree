#include "kvserver.h"

namespace bilsmtree {

KVServer::KVServer() {
  lsmtree_ = new LSMTree();
  logmanager_ = new LogManager();
}

KVServer::~KVServer() {
  delete lsmtree_;
  delete logmanager_;
}

bool KVServer::Get(const Slice key, Slice& value) {
  Slice location_;
  if (lsmtree_->Get(key, location_)) {
    value = logmanager_->Get(location_);
    return true;
  }
  return false;
}

void KVServer::MinorCompact(const SkipList* sl) {
  std::vector<KV> data_ = sl->GetAll();
  // TODO: RESIZE BEFORE PUSH_BACK
  std::vector<KV> kvs_;
  for (uint i = 0; i < data_.size(); ++ i) {
    KV kv_ = data_[i];
    Slice location_ = logmanager_->Append(kv_);
    kvs_.push_back(KV(kv_.key_, location_));
  }
  lsmtree_->AddTableToL0(kvs_);
}

}