#include "tableiterator.h"

namespace bilsmtree {

TableIterator::TableIterator() {
  id_ = 0;
  kvs_.clear();
  iter_ = 0;
}

TableIterator::TableIterator(const std::string filename, FileSystem* filesystem, const size_t footer_size, LSMTreeResult *lsmtreeresult_) {
  std::stringstream ss;
  size_t file_number_ = filesystem->Open(filename, Config::FileSystemConfig::READ_OPTION);
  size_t file_size_ = filesystem->GetFileSize(file_number_);
  if (Config::SEEK_LOG)
    std::cout << "Seek Footer in TableIterator" << std::endl;
  filesystem->Seek(file_number_, file_size_ - footer_size);
  std::string offset_data_ = filesystem->Read(file_number_, 2 * sizeof(size_t));
  ss.str(offset_data_);
  lsmtreeresult_->Read();
  size_t index_offset_ = 0;
  size_t filter_offset_ = 0;
  ss >> index_offset_;
  ss >> filter_offset_;

  if (Config::SEEK_LOG)
    std::cout << "Seek Index in TableIterator" << std::endl;
  filesystem->Seek(file_number_, index_offset_);
  std::string index_data_ = filesystem->Read(file_number_, filter_offset_ - index_offset_);
  lsmtreeresult_->Read();
  // std::cout << "Index Data:" << index_data_ << std::endl;
  ss.str(index_data_);
  while (!ss.eof()) {
    size_t key_size_ = 0;
    ss >> key_size_;
    ss.get();
    char *key_ = new char[key_size_ + 1];
    ss.read(key_, sizeof(char) * key_size_);
    key_[key_size_] = '\0';
    size_t offset_ = 0;
    size_t data_block_size_ = 0;
    ss >> offset_ >> data_block_size_;
    // std::cout << "Index:" << key_size_ << "\t" << key_ << "\t" << offset_ << "\t" << data_block_size_ << std::endl;
    if (Config::SEEK_LOG)
      std::cout << "Seek Data in TableIterator" << std::endl;
    filesystem->Seek(file_number_, offset_);
    std::string block_data = filesystem->Read(file_number_, data_block_size_);
    lsmtreeresult_->Read();
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
    ss >> key_size_;
    char *key_ = new char[key_size_ + 1];
    ss.get();
    ss.read(key_, sizeof(char) * key_size_);
    key_[key_size_] = '\0';

    size_t value_size_ = 0;
    ss >> value_size_;
    char *value_ = new char[value_size_ + 1];
    ss.get();
    ss.read(value_, sizeof(char) * value_size_);
    value_[value_size_] = '\0';

    if (key_size_ > 0)
      kvs_.push_back(KV(std::string(key_), std::string(value_)));
  }
}

}