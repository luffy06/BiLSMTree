#include <cstring>

#include <iostream>
#include <string>
#include <fstream>

#include "leveldb/db.h"
#include "filesystem.h"


namespace bilsmtree {

class LogManager {
public:
  LogManager();
  ~LogManager();

  void Append(const Slice& key, const Slice& value, std::string &location);

  void Get(const std::string &location, std::string &value);

  void CollectGarbage(leveldb::DB* db, leveldb::ReadOptions roptions, leveldb::WriteOptions woptions);
private:
  const std::string LOG_PATH = "../logs/VLOG";
  const int GARBARGE_THRESOLD = 20;

  long head, tail, record_count;
  std::fstream file_handle;

  void WriteKV(const std::string &key, const std::string &value);

  void ReadKV(std::string &key, std::string &value);
};

}