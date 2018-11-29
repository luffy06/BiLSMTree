#include "table.h"

namespace bilsmtree {

Table::Table() {
  data_blocks_ = NULL;
  index_block_ = NULL;
  filter_ = NULL;
  data_block_number_ = 0;
  filesystem_ = NULL;
}

/*
* DATA BLOCK: key,\t,value,\t
* INDEX BLOCK: last_key,\t,offset\t,data_block_size\t
*/
Table::Table(const std::vector<KV>& kvs, FileSystem* filesystem) {
  // std::cout << "Create Table" << std::endl;
  // for (size_t i = 0; i < kvs.size(); ++ i)
  //   std::cout << kvs[i].key_.ToString() << "\t";
  // std::cout << std::endl;
  assert(kvs.size() > 0);
  filesystem_ = filesystem;
  // for data blocks
  std::vector<std::string> buffers_;
  // for index block
  std::vector<Slice> last_keys_;
  // for filter block
  std::vector<Slice> keys_for_filter_;

  size_t size_ = 0;
  std::stringstream ss;
  for (size_t i = 0; i < kvs.size(); ++ i) {
    KV kv_ = kvs[i];
    // std::cout << "Iterate: " << kv_.key_.ToString() << std::endl;
    keys_for_filter_.push_back(kv_.key_);
    size_t key_size_ = kv_.key_.size(); 
    size_t value_size_ = kv_.value_.size();
    size_t add_ = kv_.size() + Util::IntToString(key_size_).size() + Util::IntToString(value_size_).size() + 4;
    size_ = size_ + add_;

    ss << key_size_;
    ss << Config::DATA_SEG;
    ss.write(kv_.key_.data(), sizeof(char) * key_size_);
    ss << Config::DATA_SEG;
    ss << value_size_;
    ss << Config::DATA_SEG;
    ss.write(kv_.value_.data(), sizeof(char) * value_size_);
    ss << Config::DATA_SEG;

    if (size_ >= Config::TableConfig::BLOCKSIZE) {
      // a data block finish
      buffers_.push_back(ss.str());
      // record index for data block
      last_keys_.push_back(kvs[i].key_);
      // create a new block
      size_ = 0;
      ss.str("");
    }
  }
  if (size_ > 0) {
    buffers_.push_back(ss.str());
    last_keys_.push_back(kvs[kvs.size() - 1].key_);
  }
  
  ss.str("");
  // std::cout << "buffers_: " << buffers_.size() << std::endl;
  // std::cout << "last_keys_: " << last_keys_.size() << std::endl;
  // std::cout << "keys_for_filter_: " << keys_for_filter_.size() << std::endl;
  // write into block
  data_block_number_ = buffers_.size();
  data_blocks_ = new Block*[data_block_number_];
  size_t data_size_ = 0;
  size_t last_offset = 0;
  for (size_t i = 0; i < data_block_number_; ++ i) {
    data_blocks_[i] = new Block(buffers_[i].data(), buffers_[i].size());
    // std::cout << "Data Block:" << i << std::endl << buffers_[i] << std::endl;
    size_t data_block_size_ = buffers_[i].size();
    data_size_ = data_size_ + buffers_[i].size();
    ss << last_keys_[i].size();
    ss << Config::DATA_SEG;
    ss.write(last_keys_[i].data(), last_keys_[i].size());
    ss << Config::DATA_SEG;
    ss << last_offset;
    ss << Config::DATA_SEG;
    ss << data_block_size_;
    ss << Config::DATA_SEG;
    last_offset = data_size_;
  }
  std::string buffer_ = ss.str();
  index_block_ = new Block(buffer_.data(), sizeof(char) * buffer_.size());
  // std::cout << "Index Block:" << buffer_ << std::endl;

  std::string algorithm = Util::GetAlgorithm();
  if (algorithm == std::string("LevelDB") || algorithm == std::string("LevelDB-KV")) {
    filter_ = new BloomFilter(keys_for_filter_); // 10 bits per key 
  }
  else if (algorithm == std::string("BiLSMTree")) {
    filter_ = new CuckooFilter(keys_for_filter_);
  }
  else {
    filter_ = NULL;
    assert(false);
  }
  // std::cout << "Filter Block In Table:" << filter_->ToString() << std::endl;

  ss.str("");
  size_t index_offset_ = data_size_;
  size_t filter_offset_ = data_size_ + index_block_->size();
  ss << index_offset_;
  ss << Config::DATA_SEG;
  ss << filter_offset_;
  ss << Config::DATA_SEG;
  footer_data_ = ss.str();

  meta_ = Meta();
  meta_.smallest_ = kvs[0].key_;
  meta_.largest_ = kvs[kvs.size() - 1].key_;
  meta_.level_ = 0;
  meta_.file_size_ = data_size_ + index_block_->size() + filter_->ToString().size() + footer_data_.size();
  meta_.footer_size_ = footer_data_.size();
  // std::cout << "file size in table:" << meta_.file_size_ << std::endl;
  // std::cout << "[" << meta_.smallest_.ToString() << "\t" << meta_.largest_.ToString() << "]" << std::endl;
  // std::cout << "DataSize:" << data_size_ << std::endl;
  // std::cout << "IndexSize:" << index_block_->size() << std::endl;
  // std::cout << "FilterSize:" << filter_->ToString().size() << std::endl;
  // std::cout << "FooterSize:" << footer_data_.size() << std::endl;
  // std::cout << "Create Table Success" << std::endl;
}

Table::~Table() {
  delete filter_;
  delete index_block_;
  delete[] data_blocks_;
}

Meta Table::GetMeta() {
  return meta_;
}

void Table::DumpToFile(const std::string filename, LSMTreeResult* lsmtreeresult) {
  // std::cout << "DumpToFile " << filename << std::endl;
  filesystem_->Create(filename);
  filesystem_->Open(filename, Config::FileSystemConfig::WRITE_OPTION);
  size_t wt = 0;
  std::stringstream ss;
  for (size_t i = 0; i < data_block_number_; ++ i) {
    ss << std::string(data_blocks_[i]->data(), data_blocks_[i]->size());
    if (ss.str().size() >= Config::TableConfig::BUFFER_SIZE) {
      std::string buffer = ss.str();
      ss.str("");
      wt ++;
      filesystem_->Write(filename, buffer.data(), buffer.size());
    }
    lsmtreeresult->Write();
  }
  // std::cout << "Write Index Block " << std::endl;
  ss << std::string(index_block_->data(), index_block_->size());
  if (ss.str().size() >= Config::TableConfig::BUFFER_SIZE) {
    std::string buffer = ss.str();
    ss.str("");
    wt ++;
    filesystem_->Write(filename, buffer.data(), buffer.size());
  }
  lsmtreeresult->Write();
  std::string filter_data = filter_->ToString();
  // std::cout << "Write Filter Block " << std::endl;
  ss << filter_data;
  if (ss.str().size() >= Config::TableConfig::BUFFER_SIZE) {
    std::string buffer = ss.str();
    ss.str("");
    wt ++;
    filesystem_->Write(filename, buffer.data(), buffer.size());
  }
  lsmtreeresult->Write();
  // std::cout << "Write Footer Block " << footer_data_ << std::endl;
  ss << footer_data_;
  if (ss.str().size() >= 0) {
    std::string buffer = ss.str();
    ss.str("");
    wt ++;
    filesystem_->Write(filename, buffer.data(), buffer.size());
  }
  filesystem_->Close(filename);
  // std::cout << "DumpToFile " << wt << " Times Write" << std::endl;
}

}