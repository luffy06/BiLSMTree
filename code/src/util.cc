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
  size_t mem_size_ = Config::CacheServerConfig::MEMORY_SIZE / 2;
  if (algo == std::string("BiLSMTreePro")) {
    size_t lru_size_ = Config::CacheServerConfig::MEMORY_SIZE * Config::CacheServerConfig::LRU2Q_RATE / (Config::CacheServerConfig::LRU2Q_RATE + 1);
    mem_size_ = Config::CacheServerConfig::MEMORY_SIZE - lru_size_;
    mem_size_ = mem_size_ / Util::GetMemTableNumb();
  }
  else if (algo == std::string("BiLSMTree-Direct")) {
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
  size_t numb = 2;
  if (algo == std::string("BiLSMTreePro"))
    numb = Config::CacheServerConfig::IMM_NUMB + 1;
  else if (algo == std::string("BiLSMTree-Direct"))
    numb = 0;
  return numb;
}

size_t Util::GetSSTableSize() {
  std::string algo = Util::GetAlgorithm();
  if (algo == std::string("LevelDB") || algo == std::string("BiLSMTree-KV"))
    return Config::TableConfig::LEVELDB_TABLE_SIZE;
  else if (algo == std::string("Wisckey"))
    return Config::TableConfig::WISCKEY_TABLE_SIZE;
  else if (algo == std::string("BiLSMTree") || algo == std::string("BiLSMTree-Ext"))
    return Config::TableConfig::BILSMTREE_TABLE_SIZE;
  else if (algo == std::string("BiLSMTree-Direct"))
    return Config::TableConfig::BILSMTREE_DIRECT_TABLE_SIZE;
  return 2 * 1024 * 1024;
}

size_t Util::GetBlockSize() {
  std::string algo = Util::GetAlgorithm();
  size_t table_size_ = GetSSTableSize();
  size_t block_size_ = 512;
  if (algo == std::string("LevelDB") || algo == std::string("BiLSMTree-KV"))
    block_size_ = Config::TableConfig::LEVELDB_BLOCK_SIZE;
  else if (algo == std::string("Wisckey"))
    block_size_ = Config::TableConfig::WISCKEY_BLOCK_SIZE;
  else if (algo == std::string("BiLSMTree") || algo == std::string("BiLSMTree-Ext"))
    block_size_ = Config::TableConfig::BILSMTREE_BLOCK_SIZE;
  else if (algo == std::string("BiLSMTree-Direct"))
    block_size_ = Config::TableConfig::BILSMTREE_DIRECT_BLOCK_SIZE;
  return table_size_ / block_size_;
}

bool Util::CheckAlgorithm(const std::string &algo, const std::vector<std::string> &algos) {
  for (size_t i = 0; i < algos.size(); ++ i)
    if (algo == algos[i])
      return true;
  return false;
}

}