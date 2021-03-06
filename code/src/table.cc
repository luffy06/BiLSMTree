#include "table.h"

namespace bilsmtree {

Table::Table() {
}

/*
* DATA BLOCK: key,\t,value,\t
* INDEX BLOCK: last_key,\t,offset\t,data_block_size\t
*/
Table::Table(const std::vector<KV>& kvs, size_t sequence_number, 
            std::string filename, FileSystem* filesystem, 
            LSMTreeResult* lsmtreeresult, const std::vector<std::string> &bloom_algos, 
            const std::vector<std::string> &cuckoo_algos) {
  if (Config::TRACE_LOG)
    std::cout << "Create Table:" << filename << std::endl;
  assert(kvs.size() > 0);
  bool ordered = true;
  for (size_t i = 1; i < kvs.size(); ++ i) {
    if (kvs[i].key_.compare(kvs[i - 1].key_) <= 0)
      ordered = false;
  }
  assert(ordered);
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
  size_t numb_ = 0;
  std::stringstream ss;
  // write data blocks
  for (size_t i = 0; i < kvs.size(); ++ i) {
    KV kv_ = kvs[i];
    keys_for_filter_.push_back(kv_.key_);
    size_t add_ = ss.str().size();
    ss << kv_.key_.ToString();
    ss << Config::DATA_SEG;
    ss << kv_.value_.ToString();
    ss << Config::DATA_SEG;
    size_ = size_ + ss.str().size() - add_;
    numb_ = numb_ + 1;
    if (size_ >= Util::GetBlockSize()) {
      std::string block_data_ = ss.str();
      ss.str("");
      ss << numb_ << Config::DATA_SEG;
      // a data block finish
      datas_.push_back(ss.str() + block_data_);
      // record index for data block
      last_keys_.push_back(kvs[i].key_);
      // create a new block
      size_ = 0;
      numb_ = 0;
      ss.str("");
    }
  }
  if (size_ > 0) {
    std::string block_data_ = ss.str();
    ss.str("");
    ss << numb_ << Config::DATA_SEG;
    datas_.push_back(ss.str() + block_data_);
    last_keys_.push_back(kvs[kvs.size() - 1].key_);
  }
  
  // write index block
  ss.str("");
  size_t data_size_ = 0;
  size_t last_offset = 0;
  ss << datas_.size() << Config::DATA_SEG;
  for (size_t i = 0; i < datas_.size(); ++ i) {
    size_t data_block_size_ = datas_[i].size();
    data_size_ = data_size_ + datas_[i].size();

    ss << last_keys_[i].ToString();
    ss << Config::DATA_SEG;
    ss << last_offset;
    ss << Config::DATA_SEG;
    ss << data_block_size_;
    ss << Config::DATA_SEG;
    last_offset = data_size_;
  }
  index_block_ = ss.str();

  // write filter block
  std::string algo = Util::GetAlgorithm();
  Filter *filter_ = NULL;
  if (Util::CheckAlgorithm(algo, bloom_algos)) {
    filter_ = new BloomFilter(keys_for_filter_); // 10 bits per key 
    filter_block_ = filter_->ToString();
  }
  else if (Util::CheckAlgorithm(algo, cuckoo_algos)) {
    filter_ = new CuckooFilter(keys_for_filter_);
    filter_block_ = filter_->ToString();
  }
  else {
    filter_ = NULL;
    assert(false);
  }
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

  // size_t data_size_sum_ = 0;
  // for (size_t i = 0; i < datas_.size(); ++ i)
  //   data_size_sum_ = data_size_sum_ + datas_[i].size();
  // std::cout << "DATA BLOCK SIZE:" << data_size_sum_ << std::endl;
  // std::cout << "INDEX BLOCK SIZE:" << index_block_.size() << std::endl;
  // std::cout << "FILTER BLOCK SIZE:" << filter_block_.size() << std::endl;
  // std::cout << "FOOTER BLOCK SIZE:" << footer_block_.size() << std::endl;

  // write into files
  datas_.push_back(index_block_);
  datas_.push_back(filter_block_);
  datas_.push_back(footer_block_);

  if (Config::TRACE_LOG)
    std::cout << "Start Write Into Files" << std::endl;
  filesystem->Create(filename);
  filesystem->Open(filename, Config::FileSystemConfig::WRITE_OPTION);
  ss.str("");
  size_t file_size_ = 0;
  for (size_t i = 0; i < datas_.size(); ++ i) {
    ss << datas_[i];
    file_size_ = file_size_ + datas_[i].size();
    if (i == datas_.size() - 1 || ss.str().size() + datas_[i + 1].size() >= Config::BUFFER_SIZE) {
      std::string buffer = ss.str();
      ss.str("");
      filesystem->Write(filename, buffer.data(), buffer.size());
    }
    lsmtreeresult->Write(datas_[i].size()); 
  }
  filesystem->Close(filename);
  if (Config::WRITE_OUTPUT)
    std::cout << "WRITE SSTABLE " << file_size_ << " DATA " << data_size_ << " INDEX " << index_block_.size() << " FILTER " << filter_block_.size() << std::endl;
  assert(file_size_ == data_size_ + index_block_.size() + filter_block_.size() + footer_block_.size());
  // create meta
  meta_ = Meta();
  meta_.smallest_ = kvs[0].key_;
  meta_.largest_ = kvs[kvs.size() - 1].key_;
  meta_.sequence_number_ = sequence_number;
  meta_.level_ = 0;
  meta_.file_size_ = file_size_;
  meta_.footer_size_ = footer_block_.size();
  meta_.data_numb_ = kvs.size();

  if (Config::TRACE_LOG) {
    std::cout << "Create Table Success! File Size:" << meta_.file_size_ << std::endl;
    std::cout << "Range:[" << meta_.smallest_.ToString() << ",\t" << meta_.largest_.ToString() << "]" << std::endl;
  }
}

Table::~Table() {
}

Meta Table::GetMeta() {
  return meta_;
}
}