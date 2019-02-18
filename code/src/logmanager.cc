#include "logmanager.h"

namespace bilsmtree {

LogManager::LogManager(FileSystem* filesystem, LSMTreeResult *lsmtreeresult) {
  file_size_ = 0;
  record_count_ = 0;
  filesystem_ = filesystem;
  lsmtreeresult_ = lsmtreeresult;
  filesystem_->Create(Config::LogManagerConfig::LOG_PATH);
}

LogManager::~LogManager() {
}

std::vector<KV> LogManager::Append(const std::vector<KV> kvs) {
  std::vector<KV> res;
  std::string total_data_ = "";
  for (size_t i = 0; i < kvs.size(); ++ i) {
    KV kv = kvs[i];
    std::stringstream ss;
    ss << kv.key_.ToString();
    ss << Config::DATA_SEG;
    ss << kv.value_.ToString();
    ss << Config::DATA_SEG;
    std::string data = ss.str();
    ss.str("");
    ss << file_size_ + buffer_.size();
    ss << ",";
    ss << data.size();
    ss << ",";
    std::string value = ss.str();
    res.push_back(KV(kv.key_.ToString(), value));
    lsmtreeresult_->Write(kv.size());
    record_count_ = record_count_ + 1;
    buffer_ = buffer_ + data;
    if (buffer_.size() >= Config::BUFFER_SIZE) {
      if (Config::TRACE_LOG)
        std::cout << "WRITE VLOG " << buffer_.size() << std::endl;
      filesystem_->Open(Config::LogManagerConfig::LOG_PATH, Config::FileSystemConfig::APPEND_OPTION);
      filesystem_->Write(Config::LogManagerConfig::LOG_PATH, buffer_.data(), buffer_.size());
      filesystem_->Close(Config::LogManagerConfig::LOG_PATH);
      file_size_ = file_size_ + buffer_.size();
      buffer_ = "";
    }
  }
  return res;
}

Slice LogManager::Get(const Slice location) {
  std::string data = location.ToString();
  std::vector<std::string> strs;
  std::string str = "";
  for (size_t i = 0; i < data.size(); ++ i) {
    if (data[i] == ',') {
      strs.push_back(str);
      str = "";
    }
    else {
      str = str + data[i];
    }
  }
  assert(strs.size() == 2);
  size_t loc_ = Util::StringToInt(strs[0]);
  size_t size_ = Util::StringToInt(strs[1]);
  KV kv = ReadKV(loc_, size_);
  lsmtreeresult_->Read(kv.size(), "VLOG");
  return kv.value_;
}

KV LogManager::ReadKV(const size_t location, const size_t size) {
  std::string log_data_;
  size_t offset = location;
  if (offset >= file_size_) {
    offset = offset - file_size_;
    log_data_ = buffer_.substr(offset, offset + size);
  }
  else {
    filesystem_->Open(Config::LogManagerConfig::LOG_PATH, Config::FileSystemConfig::READ_OPTION);
    filesystem_->Seek(Config::LogManagerConfig::LOG_PATH, location);
    log_data_ = filesystem_->Read(Config::LogManagerConfig::LOG_PATH, size);
    filesystem_->Close(Config::LogManagerConfig::LOG_PATH);
  }
  std::stringstream ss;
  ss.str(log_data_);
  std::string key_;
  std::string value_;
  ss >> key_;
  ss >> value_;
  return KV(key_, value_);
}

}