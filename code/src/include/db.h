#ifndef BILSMTREE_DB_H
#define BILSMTREE_DB_H

#include "kvserver.h"
#include "cacheserver.h"

namespace bilsmtree {

class DB {
public:
  DB();

  ~DB();

  void Put(const std::string key, const std::string value);

  bool Get(const std::string key, std::string& value);

  void ShowResult() { 
    // std::cout << "ALGORITHM:" << Util::GetAlgorithm() << std::endl;
    std::cout << "LATENCY:" << result_->flashresult_->GetLatency() << std::endl;
    std::cout << "ERASE TIMES:" << result_->flashresult_->GetEraseTimes() << std::endl;
    std::cout << "READ FILES:" << result_->lsmtreeresult_->GetReadFiles() << std::endl;
    std::cout << "WRITE FILES:" << result_->lsmtreeresult_->GetWriteFiles() << std::endl;
    std::cout << "MINOR COMPACTION:" << result_->lsmtreeresult_->GetMinorCompactionTimes() << std::endl;
    std::cout << "MAJOR COMPACTION:" << result_->lsmtreeresult_->GetMajorCompactionTimes() << std::endl;
    std::cout << "AVERAGE CHECK TIMES:" << result_->lsmtreeresult_->GetCheckTimesAvg() << std::endl;
  }
private:
  CacheServer *cacheserver_;
  KVServer *kvserver_;
  FileSystem *filesystem_;
  Result *result_;
};

}

#endif