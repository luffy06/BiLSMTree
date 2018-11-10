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
  std::vector<int> offsets_;
  std::vector<std::string> buffers_;
  std::vector<Slice> last_keys_;
  std::vector<Slice> keys_;

  size_t size_ = 0;
  size_t total_size_ = 0;
  std::string buffer_;
  Slice last_key_ = NULL;

  for (int i = 0; i < kvs.size(); ++ i) {
    KV kv_ = kvs[i];
    keys_.push_back(kv_.key_);
    size_t add_ = kv_.size() + 2 * sizeof(size_t);
    if (size_ + add_ > Config::TableConfig::BLOCKSIZE) {
      buffers_.push_back(buffer_);
      offsets_.push_back(size_);
      last_keys_.push_back(last_key_);
      size_ = add_;
      buffer_.clear();
      std::string key_size_ = Util::IntToString(kv_.key_.size());
      std::string value_size_ = Util::IntToString(kv_.value_.size()); 
      buffer_.append(key_size_.data(), key_size_.size());
      buffer_.append(kv_.key_.data(), kv_.key_.size());
      buffer_.append(value_size_.data(), value_size_.size());
      buffer_.append(kv_.value_.data(), kv_.value_.size());
      last_key_ = kv_.key_;
    }
    else {
      std::string key_size_ = Util::IntToString(kv_.key_.size());
      std::string value_size_ = Util::IntToString(kv_.value_.size()); 
      buffer_.append(key_size_.data(), key_size_.size());
      buffer_.append(kv_.key_.data(), kv_.key_.size());
      buffer_.append(value_size_.data(), value_size_.size());
      buffer_.append(kv_.value_.data(), kv_.value_.size());
      size_ = size_ + add_;
      last_key_ = kv_.key_;
    }
  }
  if (size_ > 0) {    
    buffers_.push_back(buffer_);
    offsets_.push_back(size_);
    last_keys_.push_back(last_key_);
  }

  data_block_number_ = buffers_.size();
  data_blocks_ = new Block*[data_block_number_];
  buffer_.clear();
  for (int i = 0; i < data_block_number_; ++ i) {
    data_blocks_[i] = new Block(buffers_[i].data(), buffers_[i].size());
    std::string last_key_size_ = Util::IntToString(last_keys_[i].size());
    std::string offset_ = Util::IntToString(offsets_[i]);
    buffer_.append(last_key_size_.data(), last_key_size_.size());
    buffer_.append(last_keys_[i].data(), last_keys_[i].size());
    buffer_.append(offset_.data(), offset_.size());
  }
  index_block_ = new Block(buffer_.data(), buffer_.size());
  if (Config::algorithm == Config::LevelDB)
    filter_ = new BloomFilter(keys_); // 10 bits per key 
  else if (Config::algorithm == Config::BiLSMTree)
    filter_ = new CuckooFilter(Config::FilterConfig::CUCKOOFILTER_SIZE, keys_);
  else
    Util::Assert("Algorithm Error", false);
  
  meta_ = Meta();
  meta_.largest_ = kvs[0].key_;
  meta_.smallest_ = kvs[kvs.size() - 1].key_;
  meta_.level_ = 0;
  meta_.file_size_ = (data_block_number_ + 1) * Config::TableConfig::BLOCKSIZE + Config::TableConfig::FILTERSIZE + Config::TableConfig::FOOTERSIZE;
}

Table::~Table() {
  delete filter_;
  delete index_block_;
  delete[] data_blocks_;
}

Meta Table::GetMeta() {
  return meta_;
}

void Table::DumpToFile(const std::string filename) {
  size_t file_number_ = filesystem_->Open(filename, Config::FileSystemConfig::WRITE_OPTION);
  for (size_t i = 0; i < data_block_number_; ++ i) {
    filesystem_->Seek(file_number_, i * Config::TableConfig::BLOCKSIZE);
    filesystem_->Write(file_number_, data_blocks_[i] -> data(), data_blocks_[i] -> size());
  }
  filesystem_->Seek(file_number_, data_block_number_ * Config::TableConfig::BLOCKSIZE);
  filesystem_->Write(file_number_, index_block_ -> data(), index_block_ -> size());
  filesystem_->Seek(file_number_, (data_block_number_ + 1) * Config::TableConfig::BLOCKSIZE);
  std::string filter_data = filter_ -> ToString();
  filesystem_->Write(file_number_, filter_data.data(), filter_data.size());
  filesystem_->Close(file_number_);
}

}