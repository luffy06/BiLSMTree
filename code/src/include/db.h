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

  void StartRecord();

  void ShowResult() { 
    result_->ShowResult();
  }
private:
  CacheServer *cacheserver_;
  KVServer *kvserver_;
  FileSystem *filesystem_;
  Result *result_;
};

}

#endif