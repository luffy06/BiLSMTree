#include "datamanager.h"

namespace bilsmtree {

DataManager::DataManager(FileSystem *filesystem, LSMTreeResult* lsmtreeresult) {
  file_size_ = 0;
  total_block_number_ = 0;
  status_.clear();
  filesystem_ = filesystem;
  filesystem_->Create(GetFilename());
  lsmtreeresult_ = lsmtreeresult;
}

DataManager::~DataManager() {
}
  
std::vector<BlockMeta> DataManager::Append(const std::vector<KV>& kvs) {
  assert(kvs.size() > 0);
  std::stringstream ss;

  size_t size_ = 0;
  size_t numb_ = 0;
  std::vector<std::string> smallest_keys_;
  std::vector<std::string> largest_keys_;
  std::vector<Filter*> filters_;
  std::vector<std::string> block_datas_;

  std::string smallest_ = "";
  std::string largest_ = "";
  std::vector<Slice> keys_for_filter_;
  for (size_t i = 0; i < kvs.size(); ++ i) {
    KV kv_ = kvs[i];
    keys_for_filter_.push_back(kv_.key_);
    size_ = size_ + kv_.size() + 2 * sizeof(Config::DATA_SEG);
    numb_ = numb_ + 1;
    ss << kv_.key_.ToString() << Config::DATA_SEG << kv_.value_.ToString() << Config::DATA_SEG;
    if (smallest_ == "" || kv_.key_.compare(Slice(smallest_.data(), smallest_.size())) < 0)
      smallest_ = kv_.key_.ToString();
    if (largest_ == "" || kv_.key_.compare(Slice(largest_.data(), largest_.size())) > 0)
      largest_ = kv_.key_.ToString();

    if (i == kvs.size() - 1 || size_ >= Util::GetBlockSize()) {
      std::string block_data_ = ss.str();
      ss.str("");
      ss << numb_ << Config::DATA_SEG;
      block_datas_.push_back(ss.str() + block_data_);
      smallest_keys_.push_back(smallest_);
      largest_keys_.push_back(largest_);
      filters_.push_back(new BloomFilter(keys_for_filter_));
      // clear temporary data
      smallest_ = "";
      largest_ = "";
      keys_for_filter_.clear();
      size_ = 0;
      numb_ = 0;
      ss.str("");
    }
  }

  std::vector<BlockMeta> indexs_ = WriteBlocks(block_datas_);
  for (size_t i = 0; i < indexs_.size(); ++ i) {
    BlockMeta& bm = indexs_[i];
    bm.smallest_ = Slice(smallest_keys_[i].data(), smallest_keys_[i].size());
    bm.largest_ = Slice(largest_keys_[i].data(), largest_keys_[i].size());
    bm.filter_ = filters_[i];
  }
  return indexs_;
}

bool DataManager::Get(const Slice key, Slice& value, size_t offset, size_t block_size) {
  std::string filename = GetFilename();
  filesystem_->Open(filename, Config::FileSystemConfig::READ_OPTION);
  filesystem_->Seek(filename, offset);
  std::string block_data_ = filesystem_->Read(filename, block_size);
  filesystem_->Close(filename);
  std::stringstream ss;
  ss.str(block_data_);
  size_t n;
  ss >> n;
  for (size_t i = 0; i < n; ++ i) {
    std::string key_str, value_str;
    ss << key_str << value_str;
    Slice key_ = Slice(key_str.data(), key_str.size());
    Slice value_ = Slice(value_str.data(), value_str.size());
    if (key.compare(key_) == 0) {
      value = value_;
      return true;
    }
  }
  return false;
}

std::string DataManager::ReadBlock(const BlockMeta& bm) {
  std::string filename = GetFilename();
  filesystem_->Open(filename, Config::FileSystemConfig::READ_OPTION);
  filesystem_->Seek(filename, bm.offset_);
  std::string block_data_ = filesystem_->Read(filename, bm.block_size_);
  filesystem_->Close(filename);
  lsmtreeresult_->Read(bm.block_size_, "DATA");
  return block_data_;
}

void DataManager::Invalidate(const BlockMeta& bm) {
  status_[bm.block_numb_] = 0;
}

std::string DataManager::GetFilename() {
  return std::string("../logs/sstables/data_blocks.bdb");
}

std::vector<BlockMeta> DataManager::WriteBlocks(const std::vector<std::string>& block_datas) {
  std::vector<BlockMeta> bms;
  std::string all_block_data = "";
  for (size_t i = 0; i < block_datas.size(); ++ i) {
    all_block_data = all_block_data + block_datas[i];
    BlockMeta bm;
    bm.offset_ = file_size_;
    bm.block_size_ = block_datas[i].size();
    bm.block_numb_ = total_block_number_;
    bms.push_back(bm);
    status_.push_back(1);
    file_size_ = file_size_ + block_datas[i].size();
    total_block_number_ = total_block_number_ + 1;
  }
  std::string filename = GetFilename();
  filesystem_->Open(filename, Config::FileSystemConfig::APPEND_OPTION);
  filesystem_->Write(filename, all_block_data.data(), all_block_data.size());
  filesystem_->Close(filename);
  lsmtreeresult_->Write(all_block_data.size());
  return bms;
}
}