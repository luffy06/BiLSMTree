#include "filtermanager.h"

namespace bilsmtree {

FilterManager::FilterManager(FileSystem* filesystem) {
  file_size_ = 0;
  filesystem_ = filesystem;
  filesystem_->Create(GetFilename());
}

FilterManager::~FilterManager() {
}

std::pair<size_t, size_t> FilterManager::Append(const std::string filter_data) {
  std::string filename_ = GetFilename();
  std::pair<size_t, size_t> res = std::make_pair(file_size_ + buffer_.size(), filter_data.size());
  buffer_ = buffer_ + filter_data;
  if (buffer_.size() >= Config::BUFFER_SIZE) {
    file_size_ = file_size_ + buffer_.size();
    filesystem_->Open(filename_, Config::FileSystemConfig::APPEND_OPTION);
    filesystem_->Write(filename_, buffer_.data(), buffer_.size());
    filesystem_->Close(filename_);
    buffer_ = "";
  }
  return res;
}

std::string FilterManager::Get(size_t offset, size_t size) {
  if (offset >= file_size_) {
    offset = offset - file_size_;
    return buffer_.substr(offset, offset + size);
  }
  std::string filename_ = GetFilename();
  filesystem_->Open(filename_, Config::FileSystemConfig::READ_OPTION);
  filesystem_->Seek(filename_, offset);
  std::string filter_data_ = filesystem_->Read(filename_, size);
  filesystem_->Close(filename_);
  return filter_data_;
}

std::string FilterManager::GetFilename() {
  return "../logs/filters.log";
}
}