#include <cassert>
#include <ctime>

#include <set>
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
  size_t load_number = 50000;
  size_t j = 0;
  size_t random_read = 0;
  size_t random_write = 0;
  size_t sequence_write = 0;
  size_t sequence_read = 0;
  size_t scan = 0;
  size_t total_read_size = 0;
  size_t total_write_size = 0;
  while (std::cin >> op >> key >> value) {
    if (bilsmtree::Config::TRACE_LOG)
      std::cout << "RUN " << j << "\tOp:" << op << std::endl;
    if (j == load_number) {
      db->StartRecord();
    }
    if (op == "INSERT") {
      if (j >= load_number) {
        sequence_write ++;
        total_write_size = total_write_size + key.size() + value.size();
      }
      db->Put(key, value);
    }
    else if (op == "UPDATE") {
      if (j >= load_number) {
        random_write ++;
        total_write_size = total_write_size + key.size() + value.size();
      }
      db->Put(key, value);
    }
    else if (op == "READ") {
      if (j >= load_number) {
        random_read ++;
        total_read_size = total_read_size + key.size();
      }
      db->Get(key, db_value);
    }
    else if (op == "SCAN") {
      std::string st_key = key;
      std::string ed_key = StringAddOne(value);
      if (bilsmtree::Config::TRACE_LOG)
        std::cout << "SCAN FROM:" << st_key << "\tTO:" << value << std::endl;
      if (j >= load_number)
        scan ++;
      size_t i = 0;
      for (std::string key = st_key; key != ed_key && i < bilsmtree::Config::MAX_SCAN_NUMB; key = StringAddOne(key), ++ i) {
        if (j >= load_number) {
          sequence_read ++;
          total_read_size = total_read_size + key.size();
        }
        db->Get(key, db_value);
      }
    }
    ++ j;
  }
  size_t total = random_read + random_write + sequence_read + sequence_write;
  std::cout << "READ:" << random_read << "\tUPDATE:" << random_write << "\tINSERT:" << sequence_write << "\tSCAN:" << scan << "\tSCAN_READ:" << sequence_read << std::endl;
  std::cout << "READ_RATIO:" << (random_read + sequence_read) * 1.0 / total << "\tWRITE_RATIO:" << (random_write + sequence_write) * 1.0 / total << std::endl;
  std::cout << "EXPECTED_READ:" << total_read_size << "\tEXPECTED_WRITE:" << total_write_size << std::endl;
  db->ShowResult();
  delete db;
  return 0;
}
