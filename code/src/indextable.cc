#include "indextable.h"

namespace bilsmtree {

IndexTable::IndexTable() {
}

/*
* DATA BLOCK: key,\t,value,\t
* INDEX BLOCK: last_key,\t,offset\t,data_block_size\t
*/
IndexTable::IndexTable(const std::vector<BlockMeta>& indexs_, size_t sequence_number, std::string filename, FileSystem* filesystem, LSMTreeResult* lsmtreeresult) {
  if (Config::TRACE_LOG)
    std::cout << "Create IndexTable:" << filename << std::endl;
  assert(indexs_.size() > 0);

  std::stringstream ss;
  // for index block
  std::string index_block_;
  size_t index_offset_ = 0;
  // for filter block
  size_t filter_offset_ = 0;
  // for footer block
  std::string footer_block_;
  // for write into file
  std::vector<std::string> ready_to_write_;
  size_t filter_block_size_ = 0;
  
  // write index block
  ss.str("");
  ss << indexs_.size() << Config::DATA_SEG;
  ready_to_write_.push_back(ss.str());
  index_block_ = ss.str();
  filter_block_size_ = filter_block_size_ + ss.str().size();
  for (size_t i = 0; i < indexs_.size(); ++ i) {
    BlockMeta bm = indexs_[i];
    std::string filter_data_ = bm.filter_->ToString();
    ss.str("");
    ss << filter_data_.size();
    ss << Config::DATA_SEG;
    ss << filter_data_;
    ready_to_write_.push_back(ss.str());
    filter_block_size_ = filter_block_size_ + ss.str().size();
    index_block_ = index_block_ + bm.ToString();
    delete bm.filter_;
  }
  ready_to_write_.insert(ready_to_write_.begin(), index_block_);
  filter_offset_ = index_block_.size();
  // write footer block
  ss.str("");
  ss << index_offset_;
  ss << Config::DATA_SEG;
  ss << filter_offset_;
  ss << Config::DATA_SEG;
  footer_block_ = ss.str();
  ready_to_write_.push_back(footer_block_);


  filesystem->Create(filename);
  filesystem->Open(filename, Config::FileSystemConfig::WRITE_OPTION);
  ss.str("");
  size_t file_size_ = 0;
  for (size_t i = 0; i < ready_to_write_.size(); ++ i) {
    ss << ready_to_write_[i];
    file_size_ = file_size_ + ready_to_write_[i].size();
    if (i == ready_to_write_.size() - 1 || ss.str().size() + ready_to_write_[i + 1].size() >= Config::BUFFER_SIZE) {
      std::string buffer = ss.str();
      filesystem->Write(filename, buffer.data(), buffer.size());
      ss.str("");
    }
    lsmtreeresult->Write(ready_to_write_[i].size()); 
  }
  filesystem->Close(filename);
  assert(file_size_ == filter_block_size_ + index_block_.size() + footer_block_.size());
  // create meta
  meta_ = Meta();
  meta_.smallest_ = indexs_[0].smallest_;
  meta_.largest_ = indexs_[indexs_.size() - 1].largest_;
  meta_.sequence_number_ = sequence_number;
  meta_.level_ = 0;
  meta_.file_size_ = file_size_;
  meta_.footer_size_ = footer_block_.size();
  if (Config::TRACE_LOG) {
    std::cout << "Create IndexTable Success! File Size:" << meta_.file_size_ << std::endl;
    std::cout << "Range:[" << meta_.smallest_.ToString() << ",\t" << meta_.largest_.ToString() << "]" << std::endl;
  }
}

IndexTable::~IndexTable() {
}

}