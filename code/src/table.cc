namespace bilsmtree {

Table::Table() {
  data_blocks_ = NULL;
  index_block_ = NULL;
  filter_ = NULL;
  file_number_ = -1;
  data_block_number_ = 0;
}

Table::Table(const std::vector<KV>& kvs, int sequence_number) {
  assert(kvs.size() > 0);
  std::vector<int> offsets_;
  std::vector<std::string> buffers_;
  std::vector<Slice> last_keys_;
  std::vector<Slice> keys_;

  file_number_ = sequence_number;
  size_t size_ = 0;
  std::string buffer_;
  Slice last_key_ = NULL;

  for (int i = 0; i < kvs.size(); ++ i) {
    KV kv_ = kvs[i];
    keys_.push_bakc(kv_.key_);
    size_t add_ = kv_.size();
    if (size_ + add_ > TableConfig::BLOCKSIZE) {
      buffers_.push_back(buffer_);
      offsets_.push_back(size_);
      last_keys_.push_back(last_key_);
      size_ = add_;
      buffer_.clear();
      std::string key_size_ = Util::LongToString(kv_.key_.size());
      std::string value_size_ = Util::LongToString(kv_.value_.size()); 
      buffer_.append(keysize_.data(), keysize_.size())
      buffer_.append(kv_.key_.data(), kv_.key_.size());
      buffer_.append(value_size_.data(), value_size_.size());
      buffer_.append(kv_.value_.data(), kv_.value_.size());
      last_key_ = kv_.key_;
    }
    else {
      std::string key_size_ = Util::LongToString(kv_.key_.size());
      std::string value_size_ = Util::LongToString(kv_.value_.size()); 
      buffer_.append(keysize_.data(), keysize_.size())
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

  data_block_number_ = buffers_.size_();
  data_blocks_ = new (Block *)[data_block_number_];
  buffer_.clear();
  for (int i = 0; i < data_block_number_; ++ i) {
    data_blocks_[i] = new Block(buffers_[i].data(), buffers_[i].size());
    std::string last_key_size_ = Util::LongToString(last_keys_[i].size());
    std::string offset_ = Util::LongToString(offsets_[i]);
    buffer_.append(last_key_size_.data(), last_key_size_.size());
    buffer_.append(last_keys_[i], last_keys_[i].size());
    buffer_.append(offset_.data(), offset_.size());
  }
  index_block_ = new Block(buffer_.data(), buffer_.size());
  if (GlobalConfig::algorithm == GlobalConfig::LevelDB)
    filter_ = new BloomFilter(10, keys_); // 10 bits per key 
  else if (GlobalConfig::algorithm == GlobalConfig::BiLSMTree)
    filter_ = new CuckooFilter(keys_.size() * 2, keys_);
  else
    Util::Assert("Algorithm Error", false);
}

Table::~Table() {
  delete filter_;
  delete index_block_;
  delete[] data_blocks_;
}

void Table::DumpToFile() {
  std::string filename_(TableConfig::TABLEPATH + Util::LongToString(file_number_) + ".ldb");
  
}

}