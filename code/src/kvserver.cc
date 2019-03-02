#include "kvserver.h"

namespace bilsmtree {

KVServer::KVServer(FileSystem* filesystem, LSMTreeResult* lsmtreeresult) {
  lsmtree_ = new LSMTree(filesystem, lsmtreeresult);
  logmanager_ = new LogManager(filesystem, lsmtreeresult);
}

KVServer::~KVServer() {
  delete lsmtree_;
  delete logmanager_;
}

bool KVServer::Get(const Slice key, Slice& value) {
  std::string algo = Util::GetAlgorithm();
  if (Util::CheckAlgorithm(algo, kvsep_algos)) {
    Slice location_;
    if (lsmtree_->Get(key, location_)) {
      value = logmanager_->Get(location_);
      return true;
    }
    return false;
  }
  else if (Util::CheckAlgorithm(algo, base_algos)) {
    return lsmtree_->Get(key, value);
  }
  else {
    std::cout << "Algorithm Error" << std::endl;
    assert(false);
  }
  return false;
}

void KVServer::MinorCompact(const std::vector<KV> &data) {
  std::string algo = Util::GetAlgorithm();
  if (Config::TRACE_LOG)
    std::cout << "Get All data in Immutable Memtable:" << data.size() << std::endl;
  if (Util::CheckAlgorithm(algo, kvsep_algos)) {
    std::vector<KV> kvs_ = logmanager_->Append(data);
    if (Config::TRACE_LOG)
      std::cout << "Get Locations From LogManager" << std::endl;
    lsmtree_->AddTableToL0(kvs_);
  }
  else if (Util::CheckAlgorithm(algo, base_algos)) {
    lsmtree_->AddTableToL0(data);
  }
  else {
    std::cout << "Algorithm Error" << std::endl;
    assert(false);
  }
}

}