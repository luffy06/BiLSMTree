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
  std::string algo = Util::GetAlgorithm();
  // std::cout << "Get All data in Immutable Memtable:" << data_.size() << std::endl;
  // TODO: RESIZE BEFORE PUSH_BACK
  if (algo == std::string("BiLSMTree") || algo == std::string("LevelDB-KV")) {
    std::vector<KV> kvs_ = logmanager_->Append(data_);
    lsmtree_->AddTableToL0(kvs_);
  }
  else {
    lsmtree_->AddTableToL0(data_);
  }
  // std::cout << "Ready to AddTableToL0" << std::endl;
  // std::cout << "MinorCompact Success" << std::endl;
}

}