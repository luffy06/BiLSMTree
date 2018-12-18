#include "datamanager.h"

namespace bilsmtree {

DataManager::DataManager(FileSystem *filesystem) {
  total_file_number_ = 0;
  filesystem_ = filesystem;
}

DataManager::~DataManager() {
  for (size_t i = 0; i < file_meta_.size(); ++ i) {
    std::string filename = GetFilename(file_meta_[i].file_numb_);
    filesystem_->Delete(filename);
  }
}
  
BlockMeta DataManager::Append(std::vector<KV> kvs) {
  assert(kvs.size() > 0);
  std::stringstream ss;
  std::vector<Slice> keys_for_filter_;
  for (size_t i = 0; i < kvs.size(); ++ i) {
    KV kv_ = kvs[i];
    ss << kv_.key_.ToString();
    ss << Config::DATA_SEG;
    ss << kv_.value_.ToString();
    ss << Config::DATA_SEG;
    keys_for_filter_.push_back(kv_.key_);
  }
  BlockMeta bm = WriteBlock(ss.str());
  bm.smallest_ = kvs[0].key_;
  bm.largest_ = kvs[kvs.size() - 1].key_;
  bm.filter_ = new BloomFilter(keys_for_filter_);
  return bm;
}

bool DataManager::Get(const Slice key, Slice& value, size_t file_numb, size_t offset, size_t block_size) {
  int index = FindFileMeta(file_numb);
  assert(index == -1);
  std::string filename = GetFilename(file_numb);
  filesystem_->Open(filename);
  filesystem_->Seek(offset);
  std::string block_data_ = filesystem_->Read(filename, block_size);
  filesystem_->Close(filename);
  std::stringstream ss;
  ss.str(block_data_);
  while (true) {
    std::string key_str, value_str;
    ss << key_str << value_str;
    Slice key_ = Slice(key_str.data(), key_str.size());
    Slice value_ = Slice(value_str.data(), value_str.size());
    if (key.compare(key_) == 0) {
      value = value_;
      return true;
    }
    if (ss.tellg() == -1)
      break;
  }
  return false;
}

std::string DataManager::ReadBlock(BlockMeta bm) {
  int index = FindFileMeta(bm.file_numb_);
  assert(index == -1);
  std::string filename = GetFilename(bm.file_numb_);
  filesystem_->Open(filename);
  filesystem_->Seek(bm.offset_);
  std::string block_data_ = filesystem_->Read(filename, bm.block_size_);
  filesystem_->Close(filename);
  return block_data_;
}

void DataManager::Invalidate(BlockMeta bm) {
  int index = FindFileMeta(bm.file_numb_);
  assert(index != -1);
  file_meta_[index].block_status_ &= (~(1 << bm.block_numb_));
  if (file_meta_[index].block_status_ == 0) {
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

BlockMeta DataManager::WriteBlock(std::string block_data) {
  size_t file_numb_ = 0;
  if (total_file_number_ != 0)
    file_numb = total_file_number_ - 1;
  int index = FindFileMeta(file_numb_)
  assert(index != -1);
  if (file_meta_[index].block_numb_ >= MAX_BLOCK_NUMB) {
    file_numb_ = GetFileNumber();
    index = FindFileMeta(file_numb_);
    assert(index != -1);
  }
  std::string filename = GetFilename(file_numb_);
  filesystem_->Open(filename);
  filesystem_->Seek(filename, file_meta_[index].file_size_);
  filesystem_->Write(filename, block_data_.data(), block_data_.size());
  filesystem_->Close(filename);
  file_meta_[index].file_size_ = file_meta_[index].file_size_ + block_data_.size();
  file_meta_[index].block_numb_ = file_meta_[index].block_numb_ + 1;
}

}