#include "util.h"

namespace bilsmtree {

Util::Util() {

}

Util::~Util() {

}

std::string Util::IntToString(uint value) {
  std::stringstream ss;
  std::string buf;
  ss << value;
  ss >> buf;
  return buf;
}

uint Util::StringToInt(const std::string value) {
  std::stringstream ss;
  uint buf;
  ss << value;
  ss >> buf;
  return buf;
}

bool Util::ExistFile(const std::string filename) {
  std::fstream f(filename, std::ios::in);
  bool res = f.is_open();
  f.close();
  return res;
}

void Util::Assert(const char* message, bool condition) {
  if (!condition)
    std::cout << message << std::endl;
  assert(condition);
}

}