#include <iostream>
#include <string>

#include "leveldb/db.h"
#include "logmanager.h"

class KVServer {
public:
  KVServer();
  ~KVServer();

  void Put(const std::string &key, const std::string &value);
  void Get(const std::string &key, std::string &value);
  void Delete(const std::string &key);
private:
  const std::string DB_PATH = "../logs/testdb";

  leveldb::DB* db;
  LogManager* log_manager;
  leveldb::Options options;
  leveldb::ReadOptions roptions;
  leveldb::WriteOptions woptions;


};