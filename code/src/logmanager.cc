#include "logmanager.h"

namespace bilsmtree {

LogManager::LogManager() {
  head_ = tail_ = record_count_ = 0;
}

LogManager::~LogManager() {
  head_ = tail_ = record_count_ = 0;
}

Slice LogManager::Append(const KV& kv) {
  Slice location_ = Util::IntToString(tail_);
  WriteKV(kv);
  record_count = record_count + 1;
  return location_;
}

Slice LogManager::Get(const Slice& location) {
  size_t loc = Util::StringToInt(location.ToString());
  KV kv = ReadKV(loc);
  return kv.value_;
}

void LogManager::WriteKV(const KV& kv) {
  std::string key_size_ = Util::IntToString(kv.key_.size());
  std::string value_size_ = Util::IntToString(kv.value_.size());
  size_t file_number_ = FileSystem::Open(LOG_PATH, FileSystemConfig::APPEND_OPTION);
  FileSystem::Write(file_number_, key_size_.data(), key_size_.size());
  FileSystem::Write(file_number_, kv.key_.data(), kv.key_.size());
  FileSystem::Write(file_number_, value_size_.data(), value_size_.size());
  FileSystem::Write(file_number_, kv.value_.data(), kv.value_.size());  
  tail_ = tail_ + kv.size() + 2 * sizeof(size_t);
  FileSystem::Close(file_number_);
}

KV LogManager::ReadKV(const size_t location) {
  size_t file_number_ = FileSystem::Open(LOG_PATH, FileSystemConfig::READ_OPTION);
  FileSystem::Seek(file_number_, location);
  std::string key_size_str_ = FileSystem::Read(file_number_, sizeof(size_t));
  size_t key_size_ = Util::IntToString(key_size_str_);
  Slice key_ = FileSystem::Read(file_number_, key_size_);
  std::string value_size_str_ = FileSystem::Read(file_number_, sizeof(size_t));
  size_t value_size_ = Util::IntToString(value_size_str_);
  Slice value_ = FileSystem::Read(file_number_, value_size_);
  return KV(key_, value_);
}

}