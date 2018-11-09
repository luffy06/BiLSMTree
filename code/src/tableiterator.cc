#include "tableiterator.h"

namespace bilsmtree {

TableIterator::TableIterator() {
  id_ = 0;
  kvs_.clear();
  iter_ = 0;
}

TableIterator::TableIterator(const std::string filename) {
  size_t file_number_ = FileSystem::Open(filename, Config::FileSystemConfig::READ_OPTION);
  size_t file_size_ = FileSystem::GetFileSize(file_number_);
  FileSystem::Seek(file_number_, file_size_ - Config::TableConfig::FOOTERSIZE);
  size_t index_offset_ = Util::StringToInt(FileSystem::Read(file_number_, sizeof(size_t)));
  size_t filter_offset_ = Util::StringToInt(FileSystem::Read(file_number_, sizeof(size_t)));
  FileSystem::Seek(file_number_, index_offset_);
  std::string index_data_ = FileSystem::Read(file_number_, filter_offset_ - index_offset_);
  std::istringstream is(index_data_);
  char temp[1000];
  while (!is.eof()) {
    is.read(temp, sizeof(size_t));
    size_t key_size = Util::StringToInt(std::string(temp));
    is.read(temp, key_size);
    is.read(temp, sizeof(size_t));
    size_t offset = Util::StringToInt(std::string(temp));
    FileSystem::Seek(file_number_, offset);
    std::string block_data = FileSystem::Read(file_number_, Config::TableConfig::BLOCKSIZE);
    ParseBlock(block_data);
  }
  FileSystem::Close(file_number_);
  id_ = 0;
  iter_ = 0;
}

TableIterator::~TableIterator() {
}

void TableIterator::ParseBlock(const std::string block_data) {
  std::istringstream is(block_data);
  char temp[1000];
  while (!is.eof()) {
    KV kv;
    is.read(temp, sizeof(size_t));
    size_t size = Util::StringToInt(std::string(temp));
    is.read(temp, size);
    kv.key_ = Slice(temp);
    is.read(temp, sizeof(size_t));
    size = Util::StringToInt(std::string(temp));
    is.read(temp, size);
    kv.value_ = Slice(temp);
    kvs_.push_back(kv);
  }
}

bool TableIterator::HasNext() {
  return iter_ < kvs_.size();
}

KV TableIterator::Next() {
  return kvs_[iter_++];
}

KV TableIterator::Current() {
  return kvs_[iter_];
}

void TableIterator::SetId(size_t id) {
  id_ = id;
}
}