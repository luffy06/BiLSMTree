#include "tableiterator.h"

namespace bilsmtree {

TableIterator::TableIterator() {
  id_ = 0;
  kvs_.clear();
  iter_ = 0;
}

TableIterator::TableIterator(const std::string filename, FileSystem* filesystem) {
  std::stringstream ss;
  size_t file_number_ = filesystem->Open(filename, Config::FileSystemConfig::READ_OPTION);
  size_t file_size_ = filesystem->GetFileSize(file_number_);
  filesystem->Seek(file_number_, file_size_ - Config::TableConfig::FOOTERSIZE);
  size_t index_offset_ = 0;
  size_t filter_offset_ = 0;
  ss.str(filesystem->Read(file_number_, 2 * sizeof(size_t)));
  ss.read((char *)&index_offset_, sizeof(size_t));
  ss.read((char *)&filter_offset_, sizeof(size_t));
  filesystem->Seek(file_number_, index_offset_);
  std::string index_data_ = filesystem->Read(file_number_, filter_offset_ - index_offset_);
  ss.str(index_data_);  

  while (!ss.eof()) {
    size_t key_size_ = 0;
    ss.read((char *)&key_size_, sizeof(size_t));
    char *key_ = new char[key_size_ + 1];
    ss.read(key_, sizeof(char) * key_size_);
    key_[key_size_] = '\0';
    size_t offset = 0;
    size_t data_block_size_ = 0;
    ss.read((char *)&offset, sizeof(size_t));
    ss.read((char *)&data_block_size_, sizeof(size_t));
    filesystem->Seek(file_number_, offset);
    std::string block_data = filesystem->Read(file_number_, data_block_size_);
    ParseBlock(block_data);
  }
  filesystem->Close(file_number_);
  id_ = 0;
  iter_ = 0;
}

TableIterator::~TableIterator() {
}

void TableIterator::ParseBlock(const std::string block_data) {
  std::stringstream ss;
  ss.str(block_data);
  while (!ss.eof()) {
    size_t key_size_ = 0;
    size_t value_size_ = 0;
    char *key_ = NULL;
    char *value_ = NULL;
    ss.read((char *)&key_size_, sizeof(size_t));
    key_ = new char[key_size_ + 1];
    ss.read(key_, sizeof(char) * key_size_);
    key_[key_size_] = '\0';
    ss.read((char *)&value_size_, sizeof(size_t));
    value_ = new char[value_size_ + 1];
    ss.read(value_, sizeof(char) * value_size_);
    value_[value_size_] = '\0';
    kvs_.push_back(KV(std::string(key_), std::string(value_)));
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