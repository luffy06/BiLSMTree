#include "kvserver.h"

KVServer::KVServer() {
  options.create_if_missing = true;
  leveldb::Status s = leveldb::DB::Open(options, DB_PATH, &db);
  if (!s.ok()) {
    cout << "OPEN LEVELDB FAILED!" << endl;
    cout << "Error Message:" << s.ToString() << endl;
    exit(-1);
  }

  log_manager = new LogManager();
}

KVServer::~KVServer() {
  delete db;
  delete log_manager;
}


void KVServer::Put(const string &key, const string &value) {
  // put value into vLog
  string location;
  log_manager->Append(key, value, location);
  // put key into lsm-tree
  leveldb::Status s = db->Put(woptions, key, location);
  if (!s.ok()) {
    cout << "Put Key in LSM-Tree Failed!" << endl << "Key:" << key << endl;
    cout << "Error Message:" << s.ToString() << endl;
    exit(-1);
  }
}

void KVServer::Get(const string &key, string &value) {
  string location;
  leveldb::Status s = db->Get(roptions, key, &location);
  if (s.ok()) {
    log_manager->Get(location, value);
  }
}

void KVServer::Delete(const string &key) {
}