#include <cstring>

#include <iostream>
#include <string>
#include <fstream>

#include "leveldb/db.h"
#include "util.h"

using namespace std;

class LogManager {
public:
  LogManager();
  ~LogManager();

  void Append(const string &key, const string &value, string &location);

  void Get(const string &location, string &value);

  void CollectGarbage(leveldb::DB* db, leveldb::ReadOptions roptions, leveldb::WriteOptions woptions);
private:
  const string LOG_PATH = "../logs/VLOG";
  const int GARBARGE_THRESOLD = 20;

  long head, tail, record_count;
  fstream file_handle;

  void WriteKV(const string &key, const string &value);

  void ReadKV(string &key, string &value);
};