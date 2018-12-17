#include "tableiterator.h"

namespace bilsmtree {

TableIterator::TableIterator() {
  id_ = 0;
  kvs_.clear();
  iter_ = 0;
}

TableIterator::TableIterator(const std::string filename, FileSystem* filesystem, Meta meta, LSMTreeResult *lsmtreeresult_) {
  // std::cout << "Read:" << filename << std::endl;
  // meta.Show();
  std::stringstream ss;
  std::string algo = Util::GetAlgorithm();
  filesystem->Open(filename, Config::FileSystemConfig::READ_OPTION);
  filesystem->Seek(filename, meta.file_size_ - meta.footer_size_);
  std::string offset_data_ = filesystem->Read(filename, meta.footer_size_);
  // std::cout << "Offset In TableIterator:" << offset_data_ << std::endl;
  ss.str(offset_data_);
  lsmtreeresult_->Read();
  size_t index_offset_ = 0;
  size_t filter_offset_ = 0;
  ss >> index_offset_;
  ss >> filter_offset_;
  // std::cout << "Index Offset:" << index_offset_ << "\tFilter Offset:" << filter_offset_ << std::endl;
  assert(index_offset_ != 0);
  assert(filter_offset_ != 0);
  // std::cout << "Load Filter Data" << std::endl;
  filesystem->Seek(filename, filter_offset_);
  std::string filter_data_ = filesystem->Read(filename, meta.file_size_ - filter_offset_ - meta.footer_size_);
  // std::cout << filter_data_ << std::endl;
  lsmtreeresult_->Read();
  Filter *filter_ = NULL;
  if (algo == std::string("BiLSMTree")) {
    filter_ = new CuckooFilter(filter_data_);
  }
  else if (algo == std::string("LevelDB-KV") || algo == std::string("LevelDB")) {
    filter_ = new BloomFilter(filter_data_);
  }
  else {
    filter_ = NULL;
    assert(false);
  }

  // std::cout << "Load Index Data" << std::endl;
  filesystem->Seek(filename, index_offset_);
  std::string index_data_ = filesystem->Read(filename, filter_offset_ - index_offset_);
  lsmtreeresult_->Read();
  // std::cout << "Index Data:" << index_data_ << std::endl;
  // std::cout << "Load Data" << std::endl;
  ss.str(index_data_);
  while (true) {
    std::string largest_str;
    size_t offset_ = 0;
    size_t block_size_ = 0;
    ss >> largest_str >> offset_ >> data_block_size_;
    if (ss.tellg() == -1)
      break;
    // std::cout << "Index:" << key_size_ << "\t" << key_ << "\t" << offset_ << "\t" << data_block_size_ << std::endl;
    filesystem->Seek(filename, offset_);
    std::string block_data = filesystem->Read(filename, block_size_);
    lsmtreeresult_->Read();
    ParseBlock(block_data, filter_);
  }
  filesystem->Close(filename);
  id_ = 0;
  iter_ = 0;
  delete filter_;
}

TableIterator::~TableIterator() {
  kvs_.clear();
}

void TableIterator::ParseBlock(const std::string block_data, Filter *filter) {
  std::stringstream ss;
  std::string algo = Util::GetAlgorithm();
  ss.str(block_data);
  while (true) {
    std::string key_str, value_str;
    ss >> key_str >> value_str;
    if (ss.tellg() == -1)
      break;
    
    if (algo == std::string("BiLSMTree")) {
      if (key_str.size() > 0 && filter->KeyMatch(Slice(key_str.data(), key_str.size())))
        kvs_.push_back(KV(key_str, value_str));
    }
    else {
      kvs_.push_back(KV(key_str, value_str));
    }
  }
}

}