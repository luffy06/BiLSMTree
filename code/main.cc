#include <cassert>
#include <ctime>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "db.h"

std::string StringAddOne(std::string a) {
  char *c_str = new char[a.size() + 1];
  size_t size_ = a.size();
  size_t plus_ = 1;
  for (int i = size_ - 1; i >= 0; -- i) {
    size_t sum = (i >= 0 ? a[i] - '0' : 0) + plus_;
    if (sum >= 10) {
      plus_ = 1;
      sum = sum - 10;
    }
    else {
      plus_ = 0;
    }
    c_str[i + 1] = '0' + sum;
  }
  std::string res;
  if (plus_) {
    c_str[0] = '1';
    res = std::string(c_str, size_ + 1);
  }
  else {
    c_str[0] = '0';
    res = std::string(c_str + 1, size_);
  }
  return res;
}

int main() {
  bilsmtree::DB *db = new bilsmtree::DB();
  std::string op, key, value, db_value;
  size_t j = 0;
  while (std::cin >> op >> key >> value) {
    // std::cout << "RUN " << j << "\tOp:" << op << std::endl;
    if (op == "INSERT" || op == "UPDATE") {
      db->Put(key, value);
    }
    else if (op == "READ") {
      db->Get(key, db_value);
    }
    else if (op == "SCAN") {
      std::string st_key = key;
      std::string ed_key = StringAddOne(value);
      // std::cout << "SCAN FROM:" << st_key << "\tTO:" << value << std::endl;
      for (std::string key = st_key; key != ed_key; key = StringAddOne(key)) {
        db->Get(key, db_value);
      }
    }
    ++ j;
  }
  std::cout << "Run Success" << std::endl;
  db->ShowResult();
  delete db;
  return 0;
}
