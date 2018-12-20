#include "datamanager.h"

namespace bilsmtree {

DataManager::DataManager(FileSystem *filesystem) {
  total_file_number_ = 0;
  filesystem_ = filesystem;
  GetFileNumber();
}

DataManager::~DataManager() {
  for (size_t i = 0; i < file_meta_.size(); ++ i) {
    std::string filename = GetFilename(file_meta_[i].file_numb_);
    filesystem_->Delete(filename);
  }
}
  
BlockMeta DataManager::Append(const std::vector<KV>& kvs) {
  assert(kvs.size() > 0);
  std::stringstream ss;
  std::vector<Slice> keys_for_filter_;
  ss << kvs.size();
  ss << Config::DATA_SEG;
  for (size_t i = 0; i < kvs.size(); ++ i) {
    KV kv_ = kvs[i];
    ss << kv_.key_.ToString();
    ss << Config::DATA_SEG;
    ss << kv_.value_.ToString();
    ss << Config::DATA_SEG;
    keys_for_filter_.push_back(kv_.key_);
  }
  BlockMeta bm = WriteBlock(ss.str());
  bm.smallest_ = Slice(kvs[0].key_.data(), kvs[0].key_.size());
  bm.largest_ = Slice(kvs[kvs.size() - 1].key_.data(), kvs[kvs.size() - 1].key_.size());
  bm.filter_ = new BloomFilter(keys_for_filter_);
  return bm;
}

bool DataManager::Get(const Slice key, Slice& value, size_t file_numb, size_t offset, size_t block_size) {
  int index = FindFileMeta(file_numb);
  assert(index != -1);
  std::string filename = GetFilename(file_numb);
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
  int index = FindFileMeta(bm.file_numb_);
  assert(index != -1);
  std::string filename = GetFilename(bm.file_numb_);
  filesystem_->Open(filename, Config::FileSystemConfig::READ_OPTION);
  filesystem_->Seek(filename, bm.offset_);
  std::string block_data_ = filesystem_->Read(filename, bm.block_size_);
  filesystem_->Close(filename);
  return block_data_;
}

void DataManager::Invalidate(const BlockMeta& bm) {
  int index = FindFileMeta(bm.file_numb_);
  assert(index != -1);
  assert(bm.block_numb_ >= 0 && bm.block_numb_ < MAX_BLOCK_NUMB);
  file_meta_[index].block_status_ &= (~(((size_t)1) << bm.block_numb_));
  if (file_meta_[index].block_status_ == 0) {
    // if (Config::TRACE_LOG)
    //   std::cout << "Delete DATAFILE:" << file_meta_[index].file_numb_ << std::endl;
    filesystem_->Delete(GetFilename(file_meta_[index].file_numb_));
    file_meta_.erase(file_meta_.begin() + index);
  }
}

std::string DataManager::GetFilename(size_t file_numb) {
  char filename[100];
  sprintf(filename, "../logs/sstables/data_%zu.bdb", file_numb);
  return std::string(filename);
}

size_t DataManager::GetFileNumber() {
  if (Config::TRACE_LOG)
    std::cout << "Create New DATAFILE:" << total_file_number_ << std::endl;
  filesystem_->Create(GetFilename(total_file_number_));
  FileMeta fm = FileMeta(total_file_number_);
  file_meta_.push_back(fm);
  return total_file_number_++;
}

int DataManager::FindFileMeta(size_t file_numb) {
  // TODO: Need Binary Search?
  for (size_t i = 0; i < file_meta_.size(); ++ i)
    if (file_meta_[i].file_numb_ == file_numb)
      return i;
  return -1;
}

// void DataManager::CollectGarbage() {
//   for (size_t i = 0; i < file_meta_.size(); ++ i) {
//     FileMeta fm = file_meta_[i];
//     size_t valid_numb = 0;
//     size_t status_ = fm.block_status_;
//     while (status_) {
//       if (status_ & 1)
//         valid_numb = valid_numb + 1;
//       status_ >>= 1;
//     }
//     if (valid_numb < fm.block_numb_) {
//       for (size_t j = 0; j < fm.block_numb_; ++ j) {
//         std::string block_data_;
//         if (fm.block_status_ & (1 << j)) {
//         }
//       }
//     }
//   }
// }

BlockMeta DataManager::WriteBlock(const std::string& block_data) {
  assert(total_file_number_ != 0);
  size_t file_numb_ = total_file_number_ - 1;
  int index = FindFileMeta(file_numb_);
  assert(index != -1);
  if (file_meta_[index].block_numb_ >= MAX_BLOCK_NUMB) {
    file_numb_ = GetFileNumber();
    index = FindFileMeta(file_numb_);
    assert(index != -1);
  }
  std::string filename = GetFilename(file_numb_);
  filesystem_->Open(filename, Config::FileSystemConfig::APPEND_OPTION);
  filesystem_->Write(filename, block_data.data(), block_data.size());
  filesystem_->Close(filename);
  BlockMeta bm;
  bm.file_numb_ = file_numb_;
  bm.offset_ = file_meta_[index].file_size_;
  bm.block_size_ = block_data.size();
  bm.block_numb_ = file_meta_[index].block_numb_;
  file_meta_[index].file_size_ = file_meta_[index].file_size_ + block_data.size();
  file_meta_[index].block_numb_ = file_meta_[index].block_numb_ + 1;
  return bm;
}

void DataManager::ShowFileMeta() {
  std::cout << std::string(30, '#') << std::endl;
  std::cout << "Total File:" << file_meta_.size() << std::endl;
  for (size_t i = 0; i < file_meta_.size(); ++ i)
    std::cout << "File Numb:" << file_meta_[i].file_numb_ << "\tFile Size:" << file_meta_[i].file_size_ << "\tBlock Numb:" << file_meta_[i].block_numb_ << "\tStatus:" << file_meta_[i].Status() << std::endl;
  std::cout << std::string(30, '#') << std::endl;
}

}