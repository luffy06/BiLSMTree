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
  size_t mem_size_ = Config::CacheServerConfig::MEMORY_SIZE;
  if (algo == std::string("BiLSMTreePro")) {
    size_t lru_size_ = mem_size_ * Config::CacheServerConfig::LRU2Q_RATE / (Config::CacheServerConfig::LRU2Q_RATE + 1);
    mem_size_ = mem_size_ - lru_size_;
    mem_size_ = mem_size_ / Util::GetMemTableNumb();
  }
  else if (algo == std::string("Wisckey") || algo == std::string("LevelDB")) {
    mem_size_ = mem_size_ / 2;
  }
  else {
    mem_size_ = 0;
  }
  return mem_size_;
}

size_t Util::GetLRU2QSize() {
  size_t lru2q_size_ = Config::CacheServerConfig::MEMORY_SIZE - (Util::GetMemTableSize() * Util::GetMemTableNumb());
  return lru2q_size_;
}

size_t Util::GetMemTableNumb() {
  std::string algo = Util::GetAlgorithm();
  size_t numb = 0;
  if (algo == std::string("BiLSMTreePro"))
    numb = Config::CacheServerConfig::IMM_NUMB + 1;
  else if (algo == std::string("Wisckey") || algo == std::string("LevelDB"))
    numb = 2;
  return numb;
}

size_t Util::GetSSTableSize() {
  std::string algo = Util::GetAlgorithm();
  return 8 * 1024;
}

size_t Util::GetBlockSize() {
  size_t table_size_ = GetSSTableSize();
  return table_size_ / Config::TableConfig::BLOCK_SIZE;
}

bool Util::CheckAlgorithm(const std::string &algo, const std::vector<std::string> &algos) {
  for (size_t i = 0; i < algos.size(); ++ i)
    if (algo == algos[i])
      return true;
  return false;
}

}