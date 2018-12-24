#include "indextableiterator.h"

namespace bilsmtree {

IndexTableIterator::IndexTableIterator() {
  id_ = 0;
  bms_.clear();
  iter_ = 0;
}

IndexTableIterator::IndexTableIterator(const std::string filename, FileSystem* filesystem, Meta meta, LSMTreeResult *lsmtreeresult_) {
  std::stringstream ss;
  std::string algo = Util::GetAlgorithm();
  filesystem->Open(filename, Config::FileSystemConfig::READ_OPTION);
  filesystem->Seek(filename, meta.file_size_ - meta.footer_size_);
  std::string offset_data_ = filesystem->Read(filename, meta.footer_size_);
  // std::cout << "Offset In TableIterator:" << offset_data_ << std::endl;
  ss.str(offset_data_);
  lsmtreeresult_->Read(offset_data_.size());
  size_t index_offset_ = 0;
  size_t filter_offset_ = 0;
  ss >> index_offset_;
  ss >> filter_offset_;
  // std::cout << "Index Offset:" << index_offset_ << "\tFilter Offset:" << filter_offset_ << std::endl;
  assert(index_offset_ == 0);
  assert(filter_offset_ != 0);
  // std::cout << "Load Filter Data" << std::endl;
  filesystem->Seek(filename, filter_offset_);
  std::string filter_data_ = filesystem->Read(filename, meta.file_size_ - filter_offset_ - meta.footer_size_);
  // std::cout << filter_data_ << std::endl;
  lsmtreeresult_->Read(filter_data_.size());
  ss.str(filter_data_);
  std::vector<Filter*> filters;
  size_t numb_filter_;
  ss >> numb_filter_;
  for (size_t i = 0; i < numb_filter_; ++ i) {
    size_t size_;
    ss >> size_;
    char *filter_str = new char[size_ + 1];
    ss.read(filter_str, sizeof(char) * size_);
    filter_str[size_] = '\0';
    filter_data_ = std::string(filter_str, size_);
    delete[] filter_str;
    filters.push_back(new BloomFilter(filter_data_));
  }
  // std::cout << "Load Index Data" << std::endl;
  filesystem->Seek(filename, index_offset_);
  std::string index_data_ = filesystem->Read(filename, filter_offset_ - index_offset_);
  lsmtreeresult_->Read(index_data_.size());
  filesystem->Close(filename);
  // std::cout << "Index Data:" << index_data_ << std::endl;
  // std::cout << "Load Data" << std::endl;
  ss.str(index_data_);
  size_t n;
  ss >> n;
  for (size_t i = 0; i < n; ++ i) {
    std::string smallest_str, largest_str;
    size_t offset_ = 0;
    size_t block_size_ = 0;
    size_t block_numb_ = 0;
    ss >> smallest_str >> largest_str >> offset_ >> block_size_ >> block_numb_;
    assert(i < filters.size());
    BlockMeta bm;
    bm.filter_ = filters[i];
    bm.smallest_ = Slice(smallest_str.data(), smallest_str.size());
    bm.largest_ = Slice(largest_str.data(), largest_str.size());
    bm.offset_ = offset_;
    bm.block_size_ = block_size_;
    bm.block_numb_ = block_numb_;
    bms_.push_back(bm);
  }
  id_ = 0;
  iter_ = 0;
}

IndexTableIterator::~IndexTableIterator() {
  bms_.clear();
}

}