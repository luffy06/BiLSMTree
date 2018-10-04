#include <cassert>
#include <ctime>

#include <iostream>
#include <string>
#include <vector>

#include "leveldb/db.h"
#include "kvserver.h"

using namespace std;

const int TotalKV = 1e2;
const int KeySize = 1e1;
const int ValueSize = 1e2;

string GenerateCharacter() {
  char buf[3];
  int type = rand() % 3;
  int offset = 0;
  if (type) offset = rand() % 26;
  else offset = rand() % 10;
  if (type == 0)
    sprintf(buf, "%c", ('0' + offset));
  else if (type == 1)
    sprintf(buf, "%c", ('a' + offset));
  else
    sprintf(buf, "%c", ('A' + offset));
  return buf;
}

vector<pair<string, string>> GenerateRandomKVPairs() {
  vector<pair<string, string>> kvs;
  for (int i = 0; i < TotalKV; ++ i) {
    string key = "";
    string value = "";
    for (int j = 0; j < KeySize; ++ j)
      key = key + GenerateCharacter();
    for (int j = 0; j < ValueSize; ++ j)
      value = value + GenerateCharacter();
    kvs.push_back(make_pair(key, value));
  }
  return kvs;
}

void TestFlash() {
  cout << "TestFlash" << endl;
  Flash *flash = new Flash();

  vector<int> lba_list = {17, 17, 17, 19, 20, 20, 20, 20};
  vector<string> content = {"A", "A1", "A2", "B", "C", "C1", "C2", "C3"};
  for (int i = 0; i < lba_list.size(); ++ i) {
    int lba = lba_list[i];
    string data = content[i];
    cout << "WRITE LBA:" << lba << " CONTENT:" << data << endl;
    flash->Write(lba, data.c_str());
  }
  char *d = flash->Read(17);
  cout << "LBA:" << 17 << " CONTENT:" << d << endl; 
  d = flash->Read(20);
  cout << "LBA:" << 20 << " CONTENT:" << d << endl;   
}

void TestLevelDB() {
  cout << "TestLevelDB" << endl;
  vector<pair<string, string>> kvs = GenerateRandomKVPairs();
  
  leveldb::DB* db = nullptr;
  leveldb::Options options;
  options.create_if_missing = true;

  leveldb::Status status = leveldb::DB::Open(options, "../logs/testdb", &db);

  for (pair<string, string> kv : kvs) {
    string key = kv.first;
    string value = kv.second;
    leveldb::Status s = db->Put(leveldb::WriteOptions(), key, value);
    if (!s.ok())
      cout << "WRITE ERROR! KEY:" << key << " VALUE:" << value << endl;
  }

  bool success = true;
  for (pair<string, string> kv : kvs) {
    string key = kv.first;
    string value = kv.second;
    string db_value;
    leveldb::Status s = db->Get(leveldb::ReadOptions(), key, &db_value);
    if (!s.ok()) {
      cout << "READ ERROR! KEY:" << key << " VALUE:" << value << endl;
    }
    else if (value != db_value) {
      success = false;
      cout << "KEY:" << key << " VALUE:" << value << " DB VALUE:" << db_value << endl;
    }
  }
  if (success)
    cout << "SUCCESS" << endl;
  else
    cout << "FAILURE" << endl;
  delete db;
}

void TestKVServer() {
  cout << "TestKVServer" << endl;
  vector<pair<string, string>> kvs = GenerateRandomKVPairs();

  KVServer *kvserver = new KVServer();
  for (pair<string, string> kv : kvs) {
    string key = kv.first;
    string value = kv.second;
    kvserver->Put(key, value);
  }

  bool success = true;
  for (pair<string, string> kv : kvs) {
    string key = kv.first;
    string value = kv.second;
    string db_value;
    kvserver->Get(key, db_value);
    if (value != db_value) {
      success = false;
      cout << "VALUE DOESN'T MATCH" << endl;
      cout << "KEY:" << key << endl;
      cout << value << "$$$" << endl << value.size() << endl;
      cout << db_value << "$$$" << endl << db_value.size() << endl;
      cout << endl;
    }
  }
  if (success)
    cout << "SUCCESS" << endl;
  else
    cout << "FAILURE" << endl;
  delete kvserver;
}

void TestFileSystem() {
  cout << "TestFileSystem" << endl;
  vector<pair<string, string>> kvs = GenerateRandomKVPairs();

  FatFileSystem *fatfilesystem = new FatFileSystem();
  int i = 0;
  for (pair<string, string> kv : kvs) {
    string filename = kv.first;
    string data = kv.second;
    cout << "OPEN " << filename << " " << i++ << "/" << kvs.size() << endl;
    int fd = fatfilesystem->Open(filename, 1 << 1);
    fatfilesystem->Write(fd, data, data.size());
    fatfilesystem->Write(fd, data, data.size());
    fatfilesystem->Close(fd);
  }

  bool success = true;
  for (pair<string, string> kv : kvs) {
    string filename = kv.first;
    string data = kv.second;
    int fd = fatfilesystem->Open(filename, 1);
    string filedata;
    fatfilesystem->Read(fd, filedata, data.size());
    if (filedata != data) {
      success = false;
      cout << "DATA DOESN'T MATCH" << endl;
      cout << "FILENAME:" << filename << endl;
      cout << data << "$$$" << endl << data.size() << endl;
      cout << filedata << "$$$" << endl << filedata.size() << endl;
      cout << endl;
    }
    fatfilesystem->Close(fd);
  }
  if (success)
    cout << "SUCCESS" << endl;
  else
    cout << "FAILURE" << endl;
  delete fatfilesystem;
}

int main() {
  int seed = 1000000007;
  srand((unsigned int)seed);
  // TestFlash();
  // TestLevelDB();
  // TestKVServer();
  TestFileSystem();
  return 0;
}
