#include "logmanager.h"

namespace bilsmtree {

LogManager::LogManager() {
  head_ = tail_ = record_count_ = 0;
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
  uint loc = Util::StringToInt(location.ToString());
  KV kv = ReadKV(loc);
  return kv.value_;
}

void LogManager::WriteKV(const KV kv) {
  std::string key_size_ = Util::IntToString(kv.key_.size());
  std::string value_size_ = Util::IntToString(kv.value_.size());
  uint file_number_ = FileSystem::Open(Config::LogManagerConfig::LOG_PATH, Config::FileSystemConfig::APPEND_OPTION);
  FileSystem::Write(file_number_, key_size_.data(), key_size_.size());
  FileSystem::Write(file_number_, kv.key_.data(), kv.key_.size());
  FileSystem::Write(file_number_, value_size_.data(), value_size_.size());
  FileSystem::Write(file_number_, kv.value_.data(), kv.value_.size());  
  tail_ = tail_ + kv.size() + 2 * sizeof(uint);
  FileSystem::Close(file_number_);
}

KV LogManager::ReadKV(const uint location) {
  uint file_number_ = FileSystem::Open(Config::LogManagerConfig::LOG_PATH, Config::FileSystemConfig::READ_OPTION);
  FileSystem::Seek(file_number_, location);
  std::string key_size_str_ = FileSystem::Read(file_number_, sizeof(uint));
  uint key_size_ = Util::StringToInt(key_size_str_);
  Slice key_ = FileSystem::Read(file_number_, key_size_);
  std::string value_size_str_ = FileSystem::Read(file_number_, sizeof(uint));
  uint value_size_ = Util::StringToInt(value_size_str_);
  Slice value_ = FileSystem::Read(file_number_, value_size_);
  return KV(key_, value_);
}

}