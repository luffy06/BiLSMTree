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
  srand((unsigned int)time(NULL));
  bilsmtree::DB *db = new bilsmtree::DB();
  std::string op, key, value, db_value;
  size_t j = 0;
  size_t random_read = 0;
  size_t random_write = 0;
  size_t sequence_write = 0;
  size_t sequence_read = 0;
  size_t scan = 0;
  while (std::cin >> op >> key >> value) {
    if (bilsmtree::Config::TRACE_LOG)
      std::cout << "RUN " << j << "\tOp:" << op << std::endl;
    if (j >= 30000)
      db->StartRecord();
    if (op == "INSERT") {
      if (j >= 30000)
        sequence_write ++;
      db->Put(key, value);
    }
    else if (op == "UPDATE") {
      if (j >= 30000)
        random_write ++;
      db->Put(key, value);
    }
    else if (op == "READ") {
      if (j >= 30000)
        random_read ++;
      db->Get(key, db_value);
    }
    else if (op == "SCAN") {
      std::string st_key = key;
      std::string ed_key = StringAddOne(value);
      // std::cout << "SCAN FROM:" << st_key << "\tTO:" << value << std::endl;
      if (j >= 30000)
        scan ++;
      size_t i = 0;
      for (std::string key = st_key; key != ed_key && i < bilsmtree::Config::MAX_SCAN_NUMB; key = StringAddOne(key), ++ i) {
        if (j >= 30000)
          sequence_read ++;
        db->Get(key, db_value);
      }
    }
    ++ j;
  }
  std::cout << "READ:" << random_read << "\tUPDATE:" << random_write << "\tINSERT:" << sequence_write << "\tSCAN:" << scan << "\tSCAN_READ:" << sequence_read << std::endl;
  db->ShowResult();
  delete db;
  return 0;
}
