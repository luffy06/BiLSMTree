#include "logmanager.h"

namespace bilsmtree {

LogManager::LogManager(FileSystem* filesystem) {
  head_ = tail_ = record_count_ = 0;
  filesystem_ = filesystem;
  filesystem_->Create(Config::LogManagerConfig::LOG_PATH);
}

LogManager::~LogManager() {
  head_ = tail_ = record_count_ = 0;
}

std::vector<KV> LogManager::Append(const std::vector<KV> kvs) {
  std::vector<KV> res;
  std::string total_data_ = "";
  size_t pos = tail_;
  for (size_t i = 0; i < kvs.size(); ++ i) {
    KV kv = kvs[i];
    std::stringstream ss;
    ss << kv.key_.ToString();
    ss << Config::DATA_SEG;
    ss << kv.value_.ToString();
    ss << Config::DATA_SEG;
    std::string data = ss.str();
    ss.str("");
    ss << pos;
    ss << Config::DATA_SEG;
    ss << data.size();
    ss << Config::DATA_SEG;
    std::string value = ss.str();
    res.push_back(KV(kv.key_.ToString(), value));
    pos = pos + data.size();
    // std::cout << "DATA IN LOGMANAGER:" << data.size() << std::endl;
    total_data_ = total_data_ + data;
    record_count_ = record_count_ + 1;
  }
  filesystem_->Open(Config::LogManagerConfig::LOG_PATH, Config::FileSystemConfig::APPEND_OPTION);
  filesystem_->Write(Config::LogManagerConfig::LOG_PATH, total_data_.data(), total_data_.size());
  filesystem_->Close(Config::LogManagerConfig::LOG_PATH);
  tail_ = tail_ + total_data_.size();
  return res;
}

Slice LogManager::Get(const Slice location) {
  // std::cout << location.ToString() << std::endl;
  std::stringstream ss;
  ss.str(location.ToString());
  std::string location_;
  size_t size_ = 0;
  ss >> location_;
  ss >> size_;
  size_t loc_ = Util::StringToInt(location_);
  // std::cout << loc_ << "\t" << size_ << std::endl;
  KV kv = ReadKV(loc_, size_);
  return kv.value_;
}

size_t LogManager::WriteKV(const KV kv) {
  std::stringstream ss;
  ss << kv.key_.ToString();
  ss << Config::DATA_SEG;
  ss << kv.value_.ToString();
  ss << Config::DATA_SEG;
  std::string log_data_ = ss.str();
  // std::cout << "Write Log Data:" << log_data_ << std::endl;
  filesystem_->Open(Config::LogManagerConfig::LOG_PATH, Config::FileSystemConfig::APPEND_OPTION);
  filesystem_->Write(Config::LogManagerConfig::LOG_PATH, log_data_.data(), log_data_.size());
  filesystem_->Close(Config::LogManagerConfig::LOG_PATH);
  tail_ = tail_ + log_data_.size();
  return log_data_.size();
}

KV LogManager::ReadKV(const size_t location, const size_t size) {
  filesystem_->Open(Config::LogManagerConfig::LOG_PATH, Config::FileSystemConfig::READ_OPTION);
  filesystem_->Seek(Config::LogManagerConfig::LOG_PATH, location);
  std::string log_data_ = filesystem_->Read(Config::LogManagerConfig::LOG_PATH, size);
  filesystem_->Close(Config::LogManagerConfig::LOG_PATH);
  std::stringstream ss;
  ss.str(log_data_);
  std::string key_;
  std::string value_;
  ss >> key_;
  ss >> value_;
  return KV(key_, value_);
}

}