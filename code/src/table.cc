#include "table.h"

namespace bilsmtree {

Table::Table() {
  data_blocks_ = NULL;
  index_block_ = NULL;
  filter_ = NULL;
  data_block_number_ = 0;
  filesystem_ = NULL;
}

Table::Table(const std::vector<KV>& kvs, FileSystem* filesystem) {
  assert(kvs.size() > 0);
  filesystem_ = filesystem;
  // for data blocks
  std::vector<std::string> buffers_;
  // for index block
  std::vector<size_t> offsets_;
  std::vector<Slice> last_keys_;
  // for filter block
  std::vector<Slice> keys_for_filter_;

  // std::cout << "Initialize Blocks" << std::endl;
  size_t size_ = 0;
  std::stringstream ss;
  for (size_t i = 0; i < kvs.size(); ++ i) {
    KV kv_ = kvs[i];
    keys_for_filter_.push_back(kv_.key_);
    size_t add_ = kv_.size() + 2 * sizeof(size_t);
    if (size_ + add_ > Config::TableConfig::BLOCKSIZE) {
      // a data block finish
      buffers_.push_back(ss.str());
      // record index for data block
      offsets_.push_back(size_);
      last_keys_.push_back(kvs[i - 1].key_);
      // create a new block
      size_ = add_;
      ss.clear();
      size_t key_size_ = kv_.key_.size(); 
      size_t value_size_ = kv_.value_.size();
      ss.write((char *)&key_size_, sizeof(size_t));
      ss.write(kv_.key_.data(), sizeof(char) * key_size_);
      ss.write((char *)&value_size_, sizeof(size_t));
      ss.write(kv_.value_.data(), sizeof(char) * value_size_);
    }
    else {
      size_ = size_ + add_;
      size_t key_size_ = kv_.key_.size(); 
      size_t value_size_ = kv_.value_.size();
      ss.write((char *)&key_size_, sizeof(size_t));
      ss.write(kv_.key_.data(), sizeof(char) * key_size_);
      ss.write((char *)&value_size_, sizeof(size_t));
      ss.write(kv_.value_.data(), sizeof(char) * value_size_);
    }
  }
  if (size_ > 0) {
    buffers_.push_back(ss.str());
    offsets_.push_back(size_);
    last_keys_.push_back(kvs[kvs.size() - 1].key_);
  }
  
  ss.clear();
  // std::cout << "buffers_: " << buffers_.size() << std::endl;
  // std::cout << "offsets_: " << offsets_.size() << std::endl;
  // std::cout << "last_keys_: " << last_keys_.size() << std::endl;
  // std::cout << "keys_for_filter_: " << keys_for_filter_.size() << std::endl;
  // write into block
  data_block_number_ = buffers_.size();
  data_blocks_ = new Block*[data_block_number_];
  for (size_t i = 0; i < data_block_number_; ++ i) {
    data_blocks_[i] = new Block(buffers_[i].data(), buffers_[i].size());
    std::cout << std::string(10, '#') << "Block " << i << std::string(10, '#') << std::endl;
    std::cout << data_blocks_[i]->data() << std::endl;
    std::cout << std::string(20, '#') << std::endl;
    size_t last_key_size_ = last_keys_[i].size();
    ss.write((char *)&last_key_size_, sizeof(size_t));
    ss.write(last_keys_[i].data(), sizeof(char) * last_key_size_);
    ss.write((char *)&offsets_[i], sizeof(size_t));
  }
  std::string buffer_ = ss.str();
  index_block_ = new Block(buffer_.data(), sizeof(char) * buffer_.size());
  std::string algorithm = Util::GetAlgorithm();
  if (algorithm == std::string("LevelDB")) {
    filter_ = new BloomFilter(keys_for_filter_); // 10 bits per key 
  }
  else if (algorithm == std::string("BiLSMTree")) {
    filter_ = new CuckooFilter(keys_for_filter_);
  }
  else {
    filter_ = NULL;
    assert(false);
  }
  
  meta_ = Meta();
  meta_.largest_ = kvs[0].key_;
  meta_.smallest_ = kvs[kvs.size() - 1].key_;
  meta_.level_ = 0;
  meta_.file_size_ = data_block_number_ * Config::TableConfig::BLOCKSIZE + index_block_->size() + filter_->ToString().size() + Config::TableConfig::FOOTERSIZE;
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
  size_t file_number_ = filesystem_->Open(filename, Config::FileSystemConfig::WRITE_OPTION);
  for (size_t i = 0; i < data_block_number_; ++ i) {
    filesystem_->Seek(file_number_, i * Config::TableConfig::BLOCKSIZE);
    std::cout << "Write:" << std::endl << data_blocks_[i]->size() << std::endl << data_blocks_[i]->data() << std::endl;
    filesystem_->Write(file_number_, data_blocks_[i]->data(), data_blocks_[i]->size());
    lsmtreeresult->Write();
  }
  filesystem_->Seek(file_number_, data_block_number_ * Config::TableConfig::BLOCKSIZE);
  filesystem_->Write(file_number_, index_block_->data(), index_block_->size());
  lsmtreeresult->Write();
  filesystem_->Seek(file_number_, (data_block_number_ + 1) * Config::TableConfig::BLOCKSIZE);
  std::string filter_data = filter_->ToString();
  filesystem_->Write(file_number_, filter_data.data(), filter_data.size());
  lsmtreeresult->Write();
  filesystem_->Close(file_number_);
}

}