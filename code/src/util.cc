#include "util.h"

namespace bilsmtree {

Util::Util() {

}

Util::~Util() {

}

std::string Util::IntToString(size_t value) {
  std::stringstream ss;
  std::string buf;
  ss << value;
  ss >> buf;
  return buf;
}

size_t Util::StringToInt(const std::string value) {
  std::stringstream ss;
  size_t buf;
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

std::string Util::GetAlgorithm() {
  std::fstream f("../config.in", std::ios::in);
  std::string algorithm;
  f >> algorithm;
  f.close();
  return algorithm;
}

}