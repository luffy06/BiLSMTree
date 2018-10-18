#include <iostream>
#include <string>

#include "leveldb/db.h"
#include "logmanager.h"

namespace bilsmtree {

class KVServer {
public:
  KVServer();
  ~KVServer();

  void Put(const std::string &key, const std::string &value);
  void Get(const std::string &key, std::string &value);
  void Delete(const std::string &key);
private:
  LSMTree* lsmtree_;
  LogManager* log_manager;
};

}