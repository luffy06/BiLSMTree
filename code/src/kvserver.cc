#include "kvserver.h"

KVServer::KVServer() {
  options.create_if_missing = true;
  leveldb::Status s = leveldb::DB::Open(options, DB_PATH, &db);
  if (!s.ok()) {
    std::cout << "OPEN LEVELDB FAILED!" << std::endl;
    std::cout << "Error Message:" << s.ToString() << std::endl;
    exit(-1);
  }

  log_manager = new LogManager();
}

KVServer::~KVServer() {
  delete db;
  delete log_manager;
}


void KVServer::Put(const std::string &key, const std::string &value) {
  // put value into vLog
  std::string location;
  log_manager->Append(key, value, location);
  // put key into lsm-tree
  leveldb::Status s = db->Put(woptions, key, location);
  if (!s.ok()) {
    std::cout << "Put Key in LSM-Tree Failed!" << std::endl << "Key:" << key << std::endl;
    std::cout << "Error Message:" << s.ToString() << std::endl;
    exit(-1);
  }
}

void KVServer::Get(const std::string &key, std::string &value) {
  std::string location;
  leveldb::Status s = db->Get(roptions, key, &location);
  if (s.ok()) {
    log_manager->Get(location, value);
  }
}

void KVServer::Delete(const std::string &key) {
}