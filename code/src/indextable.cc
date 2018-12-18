#include "indextable.h"

namespace bilsmtree {

IndexTable::IndexTable() {
}

/*
* DATA BLOCK: key,\t,value,\t
* INDEX BLOCK: last_key,\t,offset\t,data_block_size\t
*/
IndexTable::IndexTable(const std::vector<KV>& kvs, size_t sequence_number, std::string filename, FileSystem* filesystem, LSMTreeResult* lsmtreeresult, DataManager* datamanager) {
  if (Config::TRACE_LOG)
    std::cout << "Create IndexTable:" << filename << std::endl;
  assert(kvs.size() > 0);

  // for data blocks
  std::vector<KV> datas_;
  // for index block
  std::vector<BlockMeta> indexs_;
  std::string index_block_;
  size_t index_offset_ = 0;
  // for filter block
  size_t filter_offset_ = 0;
  // for footer block
  std::string footer_block_;
  // for write into file
  std::vector<std::string> ready_to_write_;


  size_t size_ = 0;
  // write data blocks
  for (size_t i = 0; i < kvs.size(); ++ i) {
    KV kv_ = kvs[i];
    datas_.push_back(kv_);
    size_ = size_ + kv_.size() + 2 * sizeof(Config::DATA_SEG);

    if (size_ >= Config::TableConfig::BLOCKSIZE) {
      // a data block finish
      BlockMeta bm = datamanager->Append(datas_);
      indexs_.push_back(bm);
      datas_.clear();
      size_ = 0;
    }
  }
  if (size_ > 0) {
    BlockMeta bm = datamanager->Append(datas_);
    indexs_.push_back(bm);
  }
  
  // write index block
  ss.str("");
  ss << indexs_.size();
  ss << Config::DATA_SEG;
  ready_to_write_.push_back(ss.str());
  index_offset_ = ss.str().size();
  for (size_t i = 0; i < indexs_.size(); ++ i) {
    BlockMeta bm = indexs_[i];
    std::string filter_data_ = bm.filter_->ToString();
    ss.str("");
    ss << filter_data_.size();
    ss << Config::DATA_SEG;
    ss << filter_data_;
    ready_to_write_.push_back(ss.str());
    index_offset_ = index_offset_ + ss.str().size();
    index_block_ = index_block_ + bm.ToString();
    delete bm.filter_;
  }
  ready_to_write_.push_back(index_block_);
  // write footer block
  std::stringstream ss.str("");
  ss << index_offset_;
  ss << Config::DATA_SEG;
  ss << filter_offset_;
  ss << Config::DATA_SEG;
  footer_block_ = ss.str();
  ready_to_write_.push_back(footer_block_);

  // create meta
  meta_ = Meta();
  meta_.smallest_ = kvs[0].key_;
  meta_.largest_ = kvs[kvs.size() - 1].key_;
  meta_.sequence_number_ = sequence_number;
  meta_.level_ = 0;
  meta_.file_size_ = data_size_ + index_block_.size() + filter_block_.size() + footer_block_.size();
  meta_.footer_size_ = footer_block_.size();

  filesystem->Create(filename);
  filesystem->Open(filename, Config::FileSystemConfig::WRITE_OPTION);
  ss.str("");
  for (size_t i = 0; i < ready_to_write_.size(); ++ i) {
    ss << ready_to_write_[i];
    if (i == ready_to_write_.size() - 1 || ss.str().size() + ready_to_write_[i + 1].size() >= Config::TableConfig::BUFFER_SIZE) {
      std::string buffer = ss.str();
      ss.str("");
      filesystem->Write(filename, buffer.data(), buffer.size());
    }
    lsmtreeresult->Write(); 
  }
  filesystem->Close(filename);
  if (Config::TRACE_LOG) {
    std::cout << "Create Table Success! File Size:" << meta_.file_size_ << std::endl;
  }
}

IndexTable::~IndexTable() {
}

Meta IndexTable::GetMeta() {
  return meta_;
}
}