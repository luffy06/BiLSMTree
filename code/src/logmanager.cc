#include "logmanager.h"

LogManager::LogManager() {
  file_handle.open(LOG_PATH, ios::in);
  if (!file_handle.is_open()) {
    file_handle.open(LOG_PATH, ios::app | ios::out);
  }
  file_handle.close();
  head = tail = record_count = 0;
}

LogManager::~LogManager() {
  head = tail = record_count = 0;
}

void LogManager::Append(const string &key, const string &value, string &location) {
  file_handle.open(LOG_PATH, ios::in | ios::out | ios::binary);
  file_handle.seekp(head);
  location = Convertor::LongToString(head);
  WriteKV(key, value);
  head = file_handle.tellp();
  record_count = record_count + 1;
  file_handle.close();
}

void LogManager::Get(const string &location, string &value) {
  long loc = Convertor::StringToLong(location);
  file_handle.open(LOG_PATH, ios::in | ios::binary);
  file_handle.seekp(loc);
  string key;
  ReadKV(key, value);
  file_handle.close();
}

void LogManager::CollectGarbage(leveldb::DB* db, leveldb::ReadOptions roptions, leveldb::WriteOptions woptions) {
  file_handle.open(LOG_PATH, ios::in | ios::out);
  file_handle.seekp(tail);
  int len = record_count < GARBARGE_THRESOLD ? record_count : GARBARGE_THRESOLD;
  for (int i = 0; i < len; ++ i) {
    string key, value, db_value;
    ReadKV(key, value);
    leveldb::Status s = db->Get(roptions, key, &db_value);
    // check kv valid or not
    if (s.ok()) {
      string location;
      Append(key, value, location);
      db->Put(woptions, key, location);
    }
    tail = file_handle.tellp();
  }
  record_count = record_count - len;
  file_handle.close();
}

void LogManager::WriteKV(const string &key, const string &value) {
  int key_size = key.size();
  int value_size = value.size();
  file_handle.write((char *)&key_size, sizeof(int));
  file_handle.write((char *)&value_size, sizeof(int));
  file_handle.write(key.c_str(), sizeof(char) * key_size);
  file_handle.write(value.c_str(), sizeof(char) * value_size);
}


void LogManager::ReadKV(string &key, string &value) {
  int key_size, value_size;
  file_handle.read((char *)&key_size, sizeof(int));
  file_handle.read((char *)&value_size, sizeof(int));
  char* key_c = new char[key_size + 1];
  char* value_c = new char[value_size + 1];
  file_handle.read(key_c, sizeof(char) * key_size);
  file_handle.read(value_c, sizeof(char) * value_size); 
  // TODO: right or not?
  key_c[key_size] = '\0';
  value_c[value_size] = '\0';
  key = string(key_c);
  value = string(value_c);
}