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
  std::string algorithm = "";
  f >> algorithm;
  f.close();
  return algorithm;
}

size_t Util::GetMemTableSize() {
  std::string algo = Util::GetAlgorithm();
  size_t table_size_ = Config::ImmutableMemTableConfig::MEM_SIZE;
  if (algo == std::string("LevelDB") || algo == std::string("Wisckey")) {
    table_size_ = Config::ImmutableMemTableConfig::MEM_SIZE * (Config::CacheServerConfig::MAXSIZE + 1) + Config::LRU2QConfig::M1 + Config::LRU2QConfig::M2;
    table_size_ = table_size_ / 2;
  }
  return table_size_;
}

size_t Util::GetSSTableSize() {
  // return GetMemTableSize();
  std::string algo = Util::GetAlgorithm();
  size_t table_size_ = Config::LSMTreeConfig::TABLE_SIZE;
  return table_size_;
}

}