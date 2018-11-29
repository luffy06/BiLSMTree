#include "flash.h"

namespace bilsmtree {

Flash::Flash(FlashResult *flashresult) {
  // std::cout << "CREATING FLASH" << std::endl;
  // create block files
  for (size_t i = 0; i < Config::FlashConfig::BLOCK_NUMS; ++ i) {
    std::string block_path = GetBlockPath(i);
    std::fstream f(block_path, std::ios::app | std::ios::out);
    f.close();
  }
  
  block_info_.resize(Config::FlashConfig::BLOCK_NUMS);
  for (size_t i = 0; i < Config::FlashConfig::BLOCK_NUMS; ++ i)
    block_info_[i] = BlockInfo(i);
  page_info_.resize(Config::FlashConfig::BLOCK_NUMS);
  for (size_t i = 0; i < Config::FlashConfig::BLOCK_NUMS; ++ i)
    page_info_[i].resize(Config::FlashConfig::PAGE_NUMS);
  
  for (size_t i = 0; i < Config::FlashConfig::BLOCK_NUMS; ++ i) {
    for (size_t j = 0; j < Config::FlashConfig::PAGE_NUMS; ++ j)
      assert(page_info_[i][j].status_ == PageFree);
  }

  free_blocks_num_ = Config::FlashConfig::BLOCK_NUMS;
  free_block_tag_.resize(Config::FlashConfig::BLOCK_NUMS);
  while (!free_blocks_.empty()) free_blocks_.pop();
  for (size_t i = 0; i < Config::FlashConfig::BLOCK_NUMS; ++ i) {
    free_blocks_.push(i);
    free_block_tag_[i] = true;
  }

  flashresult_ = flashresult;
  // std::cout << "CREATE SUCCESS" << std::endl;
}
  
Flash::~Flash() {
  delete flashresult_;
}

char* Flash::Read(const size_t lba) {
  // read block num and page num from page table
  if (page_table_.find(lba) == page_table_.end()) {
    std::cout << "LBA:" << lba << " doesn't exist!" << std::endl;
    assert(false);
  }
  PBA pba = page_table_[lba];
  size_t block_num_ = pba.block_num_;
  size_t page_num_ = pba.page_num_;
  // read page
  std::pair<size_t, char*> res = ReadByPageNum(block_num_, page_num_);
  assert(lba == res.first);
  // std::cout << "Read From Flash:" << res.second << std::endl << std::endl;
  return res.second;
}

void Flash::Write(const size_t lba, const char* data) {
  // calculate block num and page num
  // std::cout << std::endl << "Write LBA:" << lba << std::endl;
  // std::cout << "Write Flash:" << data << std::endl;
  size_t block_num_ = lba / Config::FlashConfig::PAGE_NUMS;
  size_t page_num_ = lba % Config::FlashConfig::PAGE_NUMS;
  // std::cout << "BLOCK:" << block_num_ << "\tPAGE" << page_num_ << std::endl;
  // examinate block whether used or not
  // if it is replace block, find corresponding primary block
  if (block_info_[block_num_].status_ == FreeBlock) {
    // std::cout << "FreeBlock" << std::endl;
    block_info_[block_num_].status_ = PrimaryBlock;
  }
  else if (block_info_[block_num_].status_ == ReplaceBlock) {
    // std::cout << "ReplaceBlock" << std::endl;
    if (block_info_[block_num_].next_ == block_num_) {
      // have not distributed a primary block
      size_t primary_block_num_ = AssignFreeBlock();
      block_info_[block_num_].next_ = primary_block_num_;
      block_info_[primary_block_num_].prev_ = block_num_;
      block_info_[primary_block_num_].status_ = PrimaryBlock;
    }
    block_num_ = block_info_[block_num_].next_;
  }
  assert(block_info_[block_num_].status_ == PrimaryBlock);
  // std::cout << "Try " << block_num_ << std::endl;
  // examinate page whether valid or not
  if (page_info_[block_num_][page_num_].status_ != PageFree) {
    // std::cout << "Write in ReplaceBlock" << std::endl;
    // no replace block
    if (block_info_[block_num_].next_ == block_num_) {
      // std::cout << "Create new ReplaceBlock" << std::endl;
      size_t replace_block_num_ = AssignFreeBlock();
      block_info_[replace_block_num_].status_ = ReplaceBlock;
      block_info_[replace_block_num_].offset_ = 0;
      block_info_[replace_block_num_].prev_ = block_num_;
      block_info_[block_num_].next_ = replace_block_num_;

    }
    size_t replace_block_num_ = block_info_[block_num_].next_;
    // std::cout << "ReplaceBlock: " << replace_block_num_ << std::endl;
    if (block_info_[replace_block_num_].offset_ >= Config::FlashConfig::PAGE_NUMS) {
      std::pair<size_t, size_t> res = MinorCollectGarbage(block_num_);
      block_num_ = res.first;
      page_num_ = res.second;
    }
    else {
      block_num_ = replace_block_num_;
      page_num_ = block_info_[block_num_].offset_;
      block_info_[block_num_].offset_ = block_info_[block_num_].offset_ + 1;
    }
  }
  // std::cout << "Write Block_" << block_num_ << "\tPage_" << page_num_ << std::endl;
  assert(page_info_[block_num_][page_num_].status_ == PageFree);
  // write page
  WriteByPageNum(block_num_, page_num_, lba, data);
  // tag block free or not
  if (free_block_tag_[block_num_]) {
    free_block_tag_[block_num_] = false;
    free_blocks_num_ = free_blocks_num_ - 1;
  }
  // std::cout << "Left " << free_blocks_num_ << std::endl;
  // check whether start garbage collection
  if (free_blocks_num_ < Config::FlashConfig::BLOCK_COLLECTION_TRIGGER)
    MajorCollectGarbage();
}

std::pair<size_t, char*> Flash::ReadByPageNum(const size_t block_num, const size_t page_num) {
  // std::cout << "Read " << block_num << "\t" << page_num << std::endl;
  size_t page_size_ = Config::FlashConfig::PAGE_SIZE;
  char *data_ = new char[page_size_ + 1];
  size_t lba_;
  std::fstream f(GetBlockPath(block_num), std::ios::in);
  f.seekp(page_num * (page_size_ * sizeof(char) + sizeof(size_t)));
  f.read(data_, page_size_ * sizeof(char));
  f.read((char *)&lba_, sizeof(size_t));
  f.close();
  data_[page_size_] = '\0';
  // write log
  // char log[Config::FlashConfig::LOG_LENGTH];
  // sprintf(log, "READ\t%zu\t%zu\t%zu\t%s\n", lba_, block_num, page_num);
  // WriteLog(log);
  flashresult_->Read();
  return std::make_pair(lba_, data_);
}

void Flash::WriteByPageNum(const size_t block_num, const size_t page_num, const size_t lba, const char* data) {
  // std::cout << "Write Flash Block_" << block_num << "\tPage_" << page_num << std::endl;
  size_t page_size_ = Config::FlashConfig::PAGE_SIZE;
  char *temp = new char[page_size_ + 1];
  strncpy(temp, data, page_size_);
  std::fstream f(GetBlockPath(block_num), std::ios::out | std::ios::in);
  f.seekp(page_num * (page_size_ * sizeof(char) + sizeof(size_t)));
  f.write(temp, page_size_ * sizeof(char));
  f.write((char *)&lba, sizeof(size_t));
  f.close();
  delete[] temp;
  // write log
  // char log[Config::FlashConfig::LOG_LENGTH];
  // sprintf(log, "WRITE\t%zu\t%zu\t%zu\t%s\n", lba, block_num, page_num);
  // WriteLog(log);
  // make old location invalid
  if (page_table_.find(lba) != page_table_.end()) {
    size_t old_block_num_ = page_table_[lba].block_num_;
    size_t old_page_num_ = page_table_[lba].page_num_;
    page_info_[old_block_num_][old_page_num_].status_ = PageInvalid;
    // std::cout << "Set Block_" << old_block_num_ << "\tPage_" << old_page_num_ << "\tPageInvalid" << std::endl;
    block_info_[old_block_num_].valid_nums_ = block_info_[old_block_num_].valid_nums_ - 1;
    block_info_[old_block_num_].invalid_nums_ = block_info_[old_block_num_].invalid_nums_ + 1;
  }
  // std::cout << "Set Block_" << block_num << "\tPage_" << page_num << "\tPageValid" << std::endl;
  // update page info
  page_info_[block_num][page_num].status_ = PageValid;
  page_info_[block_num][page_num].lba_ = lba;
  block_info_[block_num].valid_nums_ = block_info_[block_num].valid_nums_ + 1;
  // update page table
  page_table_[lba] = PBA(block_num, page_num);
  flashresult_->Write();
}

void Flash::Erase(const size_t block_num) {
  // erase block data in file
  std::fstream f(GetBlockPath(block_num), std::ios::ate | std::ios:: out);
  f.close();
  // erase page info
  for (size_t j = 0; j < Config::FlashConfig::PAGE_NUMS; ++ j)
    page_info_[block_num][j] = PageInfo();
  // erase block info
  block_info_[block_num] = BlockInfo(block_num);
  // insert into free queue
  free_blocks_.push(block_num);
  free_block_tag_[block_num] = true;
  free_blocks_num_ = free_blocks_num_ + 1;
  // write log
  // char log[Config::FlashConfig::LOG_LENGTH];
  // sprintf(log, "ERASE\t%zu\n", block_num);
  // WriteLog(log);
  flashresult_->Erase();
}

std::pair<size_t, size_t> Flash::MinorCollectGarbage(const size_t block_num) {
  // std::cout << "MinorCollectGarbage:" << block_num << std::endl;
  assert(block_info_[block_num].status_ == PrimaryBlock);
  // std::cout << "Before Free Block Number:" << free_blocks_num_ << std::endl;
  // block_info_[block_num].Show();
  // block_info_[block_info_[block_num].next_].Show();
  size_t block_num_ = block_num;
  std::vector<size_t> erase_list_;
  size_t offset_ = 0;
  size_t cur_block_num_ = 0;
  bool block_assigned = false;
  // collect primary block
  for (size_t j = 0; j < Config::FlashConfig::PAGE_NUMS; ++ j) {
    if (page_info_[block_num_][j].status_ == PageValid) {
      // std::cout << "PrimaryBlock PageValid:" << block_num_ << "\t" << j << std::endl;
      if (!block_assigned) {
        offset_ = 0;
        // allocate a new primary block
        cur_block_num_ = AssignFreeBlock();
        block_info_[cur_block_num_].status_ = PrimaryBlock;
        block_assigned = true;
      }
      size_t lba_ = page_info_[block_num_][j].lba_;
      // read data
      std::pair<size_t, char*> res = ReadByPageNum(block_num_, j);
      assert(res.first == lba_);
      // write data
      WriteByPageNum(cur_block_num_, offset_, res.first, res.second);
      // update offset
      offset_ = offset_ + 1; 
    }
  }
  erase_list_.push_back(block_num_);
  // collect replacement block if exsits
  if (block_info_[block_num_].next_ != block_num_) {
    block_num_ = block_info_[block_num_].next_;
    for (size_t j = 0; j < Config::FlashConfig::PAGE_NUMS; ++ j) {
      if (page_info_[block_num_][j].status_ == PageValid) {
        // std::cout << "ReplaceBlock PageValid:" << block_num_ << "\t" << j << std::endl;
        if (!block_assigned || offset_ >= Config::FlashConfig::PAGE_NUMS) {
          offset_ = 0;
          // allocate a new primary block
          cur_block_num_ = AssignFreeBlock();
          block_info_[cur_block_num_].status_ = PrimaryBlock;
          block_assigned = true;
        }
        size_t lba_ = page_info_[block_num_][j].lba_;
        // read data
        std::pair<size_t, char*> res = ReadByPageNum(block_num_, j);
        assert(res.first == lba_);
        // write data
        WriteByPageNum(cur_block_num_, offset_, res.first, res.second);
        // update offset
        offset_ = offset_ + 1;
      }
    }
    erase_list_.push_back(block_num_);
  }
  // erase block
  for (size_t i = 0; i < erase_list_.size(); ++ i) {
    size_t erase_block_num_ = erase_list_[i];
    if (block_info_[erase_block_num_].prev_ != erase_block_num_) {
      size_t prev_block_num_ = block_info_[erase_block_num_].prev_;
      block_info_[prev_block_num_].next_ = prev_block_num_;
    }
    if (block_info_[erase_block_num_].next_ != erase_block_num_) {
      size_t next_block_num_ = block_info_[erase_block_num_].next_;
      block_info_[next_block_num_].prev_ = next_block_num_;
    }
    Erase(erase_block_num_);
  }
  if (!block_assigned || offset_ >= Config::FlashConfig::PAGE_NUMS) {
    // create new block
    cur_block_num_ = AssignFreeBlock();
    offset_ = 0;
    // update block info
    block_info_[cur_block_num_].status_ = PrimaryBlock;
  }
  assert(offset_ < Config::FlashConfig::PAGE_NUMS);
  // std::cout << "MinorCollectGarbage End" << std::endl;
  // std::cout << "After Free Block Number:" << free_blocks_num_ << std::endl;
  return std::make_pair(cur_block_num_, offset_);
}

void Flash::MajorCollectGarbage() {
  std::cout << "MajorCollectGarbage In Flash" << std::endl;
  std::cout << "Before Free Block Number:" << free_blocks_num_ << std::endl;
  std::vector<BlockInfo> blocks_;
  for (size_t i = 0; i < Config::FlashConfig::BLOCK_NUMS; ++ i) {
    if (block_info_[i].status_ == PrimaryBlock) {
      BlockInfo bi = block_info_[i];
      if (bi.next_ != i) {
        bi.invalid_nums_ = bi.invalid_nums_ + block_info_[bi.next_].invalid_nums_;
        bi.valid_nums_ = bi.valid_nums_ + block_info_[bi.next_].valid_nums_;
      }
      blocks_.push_back(bi);
    }
  }
  sort(blocks_.begin(), blocks_.end(), [](const BlockInfo a, const BlockInfo b)->bool {
    if (a.invalid_nums_ != b.invalid_nums_)
      return a.invalid_nums_ > b.invalid_nums_;
    return a.valid_nums_ < b.valid_nums_;
  });
  for (size_t i = 0; i < blocks_.size() && free_blocks_num_ < Config::FlashConfig::BLOCK_COLLECTION_THRESOLD; ++ i) {
    size_t block_num_ = blocks_[i].block_num_;
    MinorCollectGarbage(block_num_);
  }
  std::cout << "After Free Block Number:" << free_blocks_num_ << std::endl;
  std::cout << "MajorCollectGarbage In Flash End" << std::endl;
}

size_t Flash::AssignFreeBlock() {
  while (!free_blocks_.empty() && !free_block_tag_[free_blocks_.front()])
    free_blocks_.pop();
  assert(!free_blocks_.empty());
  size_t new_block_num_ = free_blocks_.front();
  free_blocks_.pop();
  assert(block_info_[new_block_num_].status_ == FreeBlock);
  free_block_tag_[new_block_num_] = false;
  free_blocks_num_ = free_blocks_num_ - 1;
  return new_block_num_;
}

void Flash::ShowInfo() {
  for (size_t i = 0; i < Config::FlashConfig::BLOCK_NUMS; ++ i) {
    std::cout << "block:" << block_info_[i].block_num_ << std::endl;
    std::cout << block_info_[i].next_ << "\t" << block_info_[i].offset_ << std::endl;
    std::cout << block_info_[i].invalid_nums_ << "\t" << block_info_[i].valid_nums_ << std::endl;
  }
}

}