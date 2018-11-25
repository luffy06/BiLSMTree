#include "util.h"

namespace bilsmtree {

Util::Util() {

}

Util::~Util() {

}

std::string Util::IntToString(size_t value) {
  std::stringstream ss;
  ss << value;
  return ss.str();
}

size_t Util::StringToInt(const std::string value) {
  std::stringstream ss;
  size_t buf = 0;
  ss.str(value);
  ss >> buf;
  return buf;
}

bool Util::ExistFile(const std::string filename) {
  std::fstream f(filename, std::ios::in);
  bool res = f.is_open();
  f.close();
  return res;
}

std::string Util::GetAlgorithm() {
  std::fstream f(Config::ALGO_PATH, std::ios::in);
  std::string algorithm;
  f >> algorithm;
  f.close();
  return algorithm;
}

}