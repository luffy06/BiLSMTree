#include <cassert>
#include <ctime>

#include <iostream>
#include <string>
#include <vector>

#include "db.h"

const size_t TotalKV = 100;
const size_t KeySize = 100;
const size_t ValueSize = 1000;

std::string GenerateCharacter() {
  char buf[3];
  size_t type = rand() % 3;
  size_t offset = 0;
  if (type) offset = rand() % 26;
  else offset = rand() % 10;
  if (type == 0)
    sprintf(buf, "%c", static_cast<char>('0' + offset));
  else if (type == 1)
    sprintf(buf, "%c", static_cast<char>('a' + offset));
  else
    sprintf(buf, "%c", static_cast<char>('A' + offset));
  return buf;
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
    kvs.push_back(make_pair(key, value));
  }
  return kvs;
}

int main() {
  size_t seed = 1000000007;
  srand(seed);
  bilsmtree::DB *db = new bilsmtree::DB();
  delete db;
  return 0;
}
