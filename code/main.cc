#include <cassert>
#include <ctime>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "db.h"

const size_t TotalKV = 10000;
const size_t KeySize = 1000;
const size_t ValueSize = 1000;

std::string GenerateCharacter() {
  std::stringstream ss;
  std::string d;
  size_t type = rand() % 3;
  size_t offset = 0;
  if (type) offset = rand() % 26;
  else offset = rand() % 10;
  if (type == 0)
    ss << static_cast<char>('0' + offset);
  else if (type == 1)
    ss << static_cast<char>('a' + offset);
  else
    ss << static_cast<char>('A' + offset);
  ss >> d;
  return d;
}

std::vector<std::pair<std::string, std::string>> GenerateRandomKVPairs() {
  std::vector<std::pair<std::string, std::string>> kvs;
  for (size_t i = 0; i < TotalKV; ++ i) {
    std::string key = "";
    std::string value = "";
    for (size_t j = 0; j < KeySize; ++ j)
      key = key + GenerateCharacter();
    for (size_t j = 0; j < ValueSize; ++ j)
      value = value + GenerateCharacter();
    kvs.push_back(std::make_pair(key, value));
  }
  std::cout << "GENERATE SUCCESS" << std::endl;
  return kvs;
}

bool TestFlash() {

}

bool TestFileSystem() {

}

bool TestBiList() {

}

bool TestSkipList() {

}

bool TestCuckooFilter() {

}

bool TestBloomFilter() {

}

bool TestVisitFrequency() {

}

bool TestTable() {

}

bool TestTableIterator() {

}

bool TestLRU2Q() {

}

bool TestLSMTree() {

}

bool TestLogManager() {

}

bool TestKVServer() {

}

bool TestCacheServer() {

}

bool TestDB() {
  bilsmtree::DB *db = new bilsmtree::DB();
  for (size_t i = 0; i < data.size(); ++ i) {
    // std::cout << "Key" << std::endl << data[i].first << std::endl << "Value" << std::endl << data[i].second << std::endl;
    db->Put(data[i].first, data[i].second);
  }
  bool success = true;
  for (size_t i = 0; i < data.size(); ++ i) {
    // std::cout << "Validate " << i << "\t" << data[i].first << std::endl;
    std::string db_value;
    if (!db->Get(data[i].first, db_value)) {
      success = false;
      std::cout << "Key:" << data[i].first << "\tValue:" << data[i].second << " doesn't exist" << std::endl;
    }
    else if (db_value != data[i].second) {
      success = false;
      std::cout << "Key:" << data[i].first << "\tValue:" << data[i].second << " doesn't match DBValue:" << db_value << std::endl;
    }
    else {
      std::cout << "Find Target" << std::endl;
    }
  }
  delete db;
  if (success)
    std::cout << "SUCCESS" << std::endl;
  else
    std::cout << "FAILED" << std::endl;
  return success;
}

int main() {
  size_t seed = 1000000007;
  srand(seed);
  std::vector<std::pair<std::string, std::string>> data = GenerateRandomKVPairs();
  TestBiList()
  return 0;
}
