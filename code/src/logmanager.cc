#include "logmanager.h"

namespace bilsmtree {

LogManager::LogManager(FileSystem* filesystem) {
  head_ = tail_ = record_count_ = 0;
  filesystem_ = filesystem;
}

LogManager::~LogManager() {
  head_ = tail_ = record_count_ = 0;
}

Slice LogManager::Append(const KV kv) {
  std::string location_ = Util::IntToString(tail_);
  size_t size_ = WriteKV(kv);
  record_count_ = record_count_ + 1;
  std::stringstream ss;
  ss << location_;
  ss << Config::DATA_SEG;
  ss << size_;
  ss << Config::DATA_SEG;
  return Slice(ss.str());
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
  size_t file_number_ = filesystem_->Open(Config::LogManagerConfig::LOG_PATH, Config::FileSystemConfig::APPEND_OPTION);
  filesystem_->Write(file_number_, log_data_.data(), log_data_.size());
  tail_ = tail_ + log_data_.size();
  filesystem_->Close(file_number_);
  return log_data_.size();
}

KV LogManager::ReadKV(const size_t location, const size_t size_) {
  size_t file_number_ = filesystem_->Open(Config::LogManagerConfig::LOG_PATH, Config::FileSystemConfig::READ_OPTION);
  filesystem_->Seek(file_number_, location);
  std::string log_data_ = filesystem_->Read(file_number_, size_);
  // std::cout << "Log Data:" << log_data_ << std::endl;
  filesystem_->Close(file_number_);
  std::stringstream ss;
  ss.str(log_data_);
  std::string key_;
  std::string value_;
  ss >> key_;
  ss >> value_;
  return KV(key_, value_);
}

}