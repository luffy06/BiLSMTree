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
  Slice location_ = Util::IntToString(tail_);
  WriteKV(kv);
  record_count_ = record_count_ + 1;
  return location_;
}

Slice LogManager::Get(const Slice location) {
  size_t loc = Util::StringToInt(location.ToString());
  KV kv = ReadKV(loc);
  return kv.value_;
}

void LogManager::WriteKV(const KV kv) {
  std::string key_size_ = Util::IntToString(kv.key_.size());
  std::string value_size_ = Util::IntToString(kv.value_.size());
  size_t file_number_ = filesystem_->Open(Config::LogManagerConfig::LOG_PATH, Config::FileSystemConfig::APPEND_OPTION);
  filesystem_->Write(file_number_, key_size_.data(), key_size_.size());
  filesystem_->Write(file_number_, kv.key_.data(), kv.key_.size());
  filesystem_->Write(file_number_, value_size_.data(), value_size_.size());
  filesystem_->Write(file_number_, kv.value_.data(), kv.value_.size());  
  tail_ = tail_ + kv.size() + 2 * sizeof(size_t);
  filesystem_->Close(file_number_);
}

KV LogManager::ReadKV(const size_t location) {
  size_t file_number_ = filesystem_->Open(Config::LogManagerConfig::LOG_PATH, Config::FileSystemConfig::READ_OPTION);
  filesystem_->Seek(file_number_, location);
  std::string key_size_str_ = filesystem_->Read(file_number_, sizeof(size_t));
  size_t key_size_ = Util::StringToInt(key_size_str_);
  Slice key_ = filesystem_->Read(file_number_, key_size_);
  std::string value_size_str_ = filesystem_->Read(file_number_, sizeof(size_t));
  size_t value_size_ = Util::StringToInt(value_size_str_);
  Slice value_ = filesystem_->Read(file_number_, value_size_);
  return KV(key_, value_);
}

}