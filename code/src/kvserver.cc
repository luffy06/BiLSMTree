#include "kvserver.h"

namespace bilsmtree {

KVServer::KVServer(FileSystem* filesystem, LSMTreeResult* lsmtreeresult) {
  lsmtree_ = new LSMTree(filesystem, lsmtreeresult);
  logmanager_ = new LogManager(filesystem);
}

KVServer::~KVServer() {
  delete lsmtree_;
  delete logmanager_;
}

bool KVServer::Get(const Slice key, Slice& value) {
  std::string algo = Util::GetAlgorithm();
  if (algo == std::string("BiLSMTree") || algo == std::string("LevelDB-KV")) {
    Slice location_;
    if (lsmtree_->Get(key, location_)) {
      // std::cout << "Get Location " << location_.ToString() << std::endl;
      value = logmanager_->Get(location_);
      return true;
    }
    return false;
  }
  else if (algo == std::string("LevelDB")) {
    return lsmtree_->Get(key, value);
  }
  return false;
}

void KVServer::MinorCompact(const SkipList* sl) {
  std::vector<KV> data_ = sl->GetAll();
  // std::cout << "Get All data in Immutable Memtable:" << data_.size() << std::endl;
  // TODO: RESIZE BEFORE PUSH_BACK
  std::vector<KV> kvs_;
  for (size_t i = 0; i < data_.size(); ++ i) {
    KV kv_ = data_[i];
    std::string algo = Util::GetAlgorithm();
    if (algo == std::string("BiLSMTree") || algo == std::string("LevelDB-KV")) {
      Slice location_ = logmanager_->Append(kv_);
      // std::cout << "Key:" << kv_.key_.ToString() << "\tLocation:" << location_.ToString() << std::endl;
      kvs_.push_back(KV(kv_.key_, location_));
    }
    else if (algo == std::string("LevelDB")) {
      kvs_.push_back(kv_);
    }
  }
  // std::cout << "Ready to AddTableToL0" << std::endl;
  lsmtree_->AddTableToL0(kvs_);
  // std::cout << "MinorCompact Success" << std::endl;
}

}