#include <iostream>
#include <string>

#include "leveldb/db.h"
#include "logmanager.h"

using namespace std;

class KVServer {
public:
  KVServer();
  ~KVServer();

  void Put(const string &key, const string &value);
  void Get(const string &key, string &value);
  void Delete(const string &key);
private:
  const string DB_PATH = "../logs/testdb";

  leveldb::DB* db;
  LogManager* log_manager;
  leveldb::Options options;
  leveldb::ReadOptions roptions;
  leveldb::WriteOptions woptions;


};