#include <cassert>
#include <ctime>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "db.h"

int main() {
  bilsmtree::DB *db = new bilsmtree::DB();
  std::string op, key, value, db_value;
  size_t j = 0;
  while (std::cin >> op >> key >> value) {
    // if (j % 1000 == 0) {
    //   std::cout << "RUN " << j << "\tOp:" << op << std::endl;
    //   std::cout << key << "\t" << value << std::endl;
    // }
    if (op == "INSERT" || op == "UPDATE")
      db->Put(key, value);
    else if (op == "SCAN" || op == "READ")
      db->Get(key, db_value);
    ++ j;
  }
  std::cout << "Run Success" << std::endl;
  db->ShowResult();
  delete db;
  return 0;
}
