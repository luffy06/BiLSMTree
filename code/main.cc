#include <cassert>
#include <ctime>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "db.h"

int main() {
  std::fstream f;
  size_t n = 1;
  for (size_t i = 0; i < n; ++ i) {
    std::stringstream ss;
    ss << i;
    std::string data_name = "../data/data" + ss.str() + ".in";
    f.open(data_name, std::ios::in);
    bilsmtree::DB *db = new bilsmtree::DB();
    for (size_t j = 0; !f.eof(); ++ j) {
      std::string op, key, value, db_value;
      f >> op >> key >> value;
      if (j % 1000 == 0) {
        std::cout << "RUN " << j << std::endl;
        // std::cout << "Op:" << op << "\t" << key << "\t" << value << std::endl;
      }
      if (op == "INSERT" || op == "UPDATE")
        db->Put(key, value);
      else if (op == "SCAN" || op == "READ")
        db->Get(key, db_value);
      else
        std::cout << "Operation Error:" << op << std::endl;
    }
    std::cout << "Run Success" << std::endl;
    db->ShowResult();
    delete db;
    f.close();
  }
  return 0;
}
