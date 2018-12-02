#include "table.h"

namespace bilsmtree {

Table::Table() {
}

/*
* DATA BLOCK: key,\t,value,\t
* INDEX BLOCK: last_key,\t,offset\t,data_block_size\t
*/
Table::Table(const std::vector<KV>& kvs, size_t sequence_number, std::string filename, FileSystem* filesystem, LSMTreeResult* lsmtreeresult) {
  if (Config::TRACE_LOG)
    std::cout << "Create Table:" << filename << std::endl;
  assert(kvs.size() > 0);
  // for data blocks
  std::vector<std::string> datas_;
  // for index block
  std::vector<Slice> last_keys_;
  std::string index_block_;
  // for filter block
  std::vector<Slice> keys_for_filter_;
  std::string filter_block_;
  // for footer block
  std::string footer_block_;

  size_t size_ = 0;
  std::stringstream ss;
  // write data blocks
  for (size_t i = 0; i < kvs.size(); ++ i) {
    KV kv_ = kvs[i];
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
      datas_.push_back(ss.str());
      // record index for data block
      last_keys_.push_back(kvs[i].key_);
      // create a new block
      size_ = 0;
      ss.str("");
    }
  }
  if (size_ > 0) {
    datas_.push_back(ss.str());
    last_keys_.push_back(kvs[kvs.size() - 1].key_);
  }
  
  // write index block
  ss.str("");
  size_t data_size_ = 0;
  size_t last_offset = 0;
  for (size_t i = 0; i < datas_.size(); ++ i) {
    size_t data_block_size_ = datas_[i].size();
    data_size_ = data_size_ + datas_[i].size();

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
  index_block_ = ss.str();

  // write filter block
  std::string algorithm = Util::GetAlgorithm();
  Filter *filter_ = NULL;
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
  filter_block_ = filter_->ToString();
  delete filter_;

  // write footer block
  ss.str("");
  size_t index_offset_ = data_size_;
  size_t filter_offset_ = data_size_ + index_block_.size();
  ss << index_offset_;
  ss << Config::DATA_SEG;
  ss << filter_offset_;
  ss << Config::DATA_SEG;
  footer_block_ = ss.str();

  // create meta
  meta_ = Meta();
  meta_.smallest_ = kvs[0].key_;
  meta_.largest_ = kvs[kvs.size() - 1].key_;
  meta_.sequence_number_ = sequence_number;
  meta_.level_ = 0;
  meta_.file_size_ = data_size_ + index_block_.size() + filter_block_.size() + footer_block_.size();
  meta_.footer_size_ = footer_block_.size();

  // for (size_t i = 0; i < datas_.size(); ++ i)
  //   std::cout << "DATA BLOCK SIZE:" << datas_[i].size() << std::endl;
  // std::cout << "INDEX BLOCK SIZE:" << index_block_.size() << std::endl;
  // std::cout << "FILTER BLOCK SIZE:" << filter_block_.size() << std::endl;
  // std::cout << "FOOTER BLOCK SIZE:" << footer_block_.size() << std::endl;

  // write into files
  datas_.push_back(index_block_);
  datas_.push_back(filter_block_);
  datas_.push_back(footer_block_);

  filesystem->Create(filename);
  filesystem->Open(filename, Config::FileSystemConfig::WRITE_OPTION);
  ss.str("");
  for (size_t i = 0; i < datas_.size(); ++ i) {
    ss << datas_[i];
    if (i == datas_.size() - 1 || ss.str().size() + datas_[i + 1].size() >= Config::TableConfig::BUFFER_SIZE) {
      std::string buffer = ss.str();
      ss.str("");
      filesystem->Write(filename, buffer.data(), buffer.size());
    }
    lsmtreeresult->Write(); 
  }
  filesystem->Close(filename);
  if (Config::TRACE_LOG)
    std::cout << "Create Table Success! Range:[" << meta_.smallest_.ToString() << ",\t" << meta_.largest_.ToString() << "]" << std::endl;
}

Table::~Table() {
}

Meta Table::GetMeta() {
  return meta_;
}
}