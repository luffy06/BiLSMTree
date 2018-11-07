#include "flash.h"

namespace bilsmtree {

Flash::Flash() {
  std::cout << "CREATING FLASH" << std::endl;
  std::fstream f;
  // create block files
  for (int i = 0; i < BLOCK_NUMS; ++ i) {
    std::string block_path = GetBlockPath(i);
    if (!Util::ExistFile(block_path)) {
      std::cout << "CREATING BLOCK FILE " << i << std::endl;
      f.open(block_path, std::ios::app | std::ios::out);
      f.close();
    }
  }
  
  block_info_ = new BlockInfo[BLOCK_NUMS];
  page_info_ = new PageInfo*[BLOCK_NUMS];
  for (int i = 0; i < BLOCK_NUMS; ++ i)
    page_info_[i] = new PageInfo[PAGE_NUMS];
  page_table_ = new PBA[LBA_NUMS];
  
  free_blocks_.clear();
  for (int i = 0; i < BLOCK_NUMS; ++ i)
    free_blocks_.push(i);
  std::cout << "CREATE SUCCESS" << std::endl;
}
  
Flash::~Flash() {
  // destory page_info_
  for (int i = 0; i < BLOCK_NUMS; ++ i)
    delete[] page_info_[i];
  delete[] page_info_;

  delete[] block_info_;
}

char* Flash::Read(const size_t lba) {
  // calculate block num and page num
  PBA pba = page_table_[lba];
  size_t block_num_ = pba.block_num_;
  size_t page_num_ = pba.page_num_;
  // read page
  std::pair<size_t, char*> res = ReadByPageNum(block_num, page_num);
  return res.second;
}

void Flash::Write(const size_t lba, const char* data) {
  // calculate block num and page num
  size_t block_num_ = lba / PAGE_NUMS;
  size_t page_num_ = lba % PAGE_NUMS;
  // examinate block whether used or not
  // if it is replace block, find corresponding primary block
  if (block_info_[block_num_].status_ == FreeBlock) { 
    block_info_[block_num_].status_ = PrimaryBlock;
  }
  else if (block_info_[block_num_].status_ == ReplaceBlock) {
    if (block_info_[block_num_].corresponding_ == block_num_) {
      // have not distributed a primary block
      size_t primary_block_num_ = AssignFreeBlock();
      block_info_[block_num_].corresponding_ = primary_block_num_;
      block_info_[primary_block_num_].status_ = PrimaryBlock;
    }
    block_num_ = block_info_[block_num_].corresponding_;
  }
  Util::Assert("BLOCK IS NOT PRIMARY BLOCK", block_info_[block_num_].status_ == PrimaryBlock);
  // examinate page whether valid or not
  if (page_info_[block_num_][page_num_].status_ != PageFree) {
    // no replace block
    if (block_info_[block_num_].corresponding_ == block_num_) {
      size_t _block_num_ = AssignFreeBlock();
      block_info_[block_num_].corresponding_ = _block_num_;
      block_info_[block_num_].status_ = ReplaceBlock;
      block_info_[block_num_].offset_ = 0;
    }
    block_num_ = block_info_[block_num_].corresponding_;
    if (block_info_[block_num_].offset_ >= PAGE_NUMS) {
      std::pair<size_t, size_t> res = MinorCollectGarbage(block_num_);
      block_num_ = res.first;
      page_num_ = res.second;
    }
    else {
      page_num_ = block_info_[block_num_].offset_;
      block_info_[block_num_].offset_ = block_info_[block_num_].offset_ + 1;
    }
  }
  Util::Assert("PAGE IS NOT FREE", page_info_[block_num_][page_num_].status_ = )
  // write page
  WriteByPageNum(block_num_, page_num_, lba, data);
  // make old location invalid
  if (page_table_.find(lba) != page_table_.end()) {
    size_t old_block_num_ = page_table_[lba].block_num_;
    size_t old_page_num_ = page_table_[lba].page_num_;
    page_info_[old_block_num_][old_page_num_].status_ = PageInvalid;
  }
  // update page info
  page_info_[block_num_][page_num_].status_ = PageValid;
  page_info_[block_num_][page_num_].lba_ = lba;
  // update page table
  page_table_[lba] = PBA(block_num_, page_num_);
  // check whether start garbage collection
  if (free_blocks_.size() < BLOCK_THRESOLD)
    MajorCollectGarbage();
}

std::pair<size_t, char*> Flash::ReadByPageNum(const size_t block_num, const size_t page_num) {
  char *data_ = new char[PAGE_SIZE];
  size_t lba_;
  std::fstream f(GetBlockPath(block_num), std::ios::in);
  f.seekp(page_num * (PAGE_SIZE * sizeof(char) + sizeof(size_t)));
  f.read(data_, PAGE_SIZE * sizeof(char));
  f.read((char *)&lba_, sizeof(size_t));
  f.close();
  data_[PAGE_SIZE] = '\0';
  // write log
  char log[LOG_LENGTH];
  sprintf(log, "READ\t%d\t%d\t%d\t%s\n", lba_, block_num, page_num, data_);
  WriteLog(log);
  return make_pair(lba_, data_);
}

void Flash::WriteByPageNum(const size_t block_num, const size_t page_num, const size_t lba, const char* data) {
  char *ndata = new char[PAGE_SIZE];
  strcpy(ndata, data);
  std::fstream f(GetBlockPath(block_num), std::ios::out | std::ios::in);
  f.seekp(page_num * (PAGE_SIZE * sizeof(char) + sizeof(size_t)));
  f.write(ndata, PAGE_SIZE * sizeof(char));
  f.write((char *)&lba, sizeof(size_t));
  f.close();
  // write log
  char log[LOG_LENGTH];
  sprintf(log, "WRITE\t%d\t%d\t%d\t%s\n", lba, block_num, page_num, data);
  WriteLog(log);
}

void Flash::Erase(const size_t block_num) {
  // erase block data in file
  std::fstream f(GetBlockPath(block_num), std::ios::ate | std::ios:: out);
  f.close();
  // erase page info
  for (int j = 0; j < PAGE_NUMS; ++ j)
    page_info_[block_num][j] = PageInfo();
  // erase block info
  block_info_[block_num] = BlockInfo(block_num);
  // insert into free queue
  free_blocks_.push(block_num);
  // write log
  char log[LOG_LENGTH];
  sprintf(log, "ERASE\t%d\n", block_num);
  WriteLog(log);
}

void Flash::MinorCollectGarbage(const size_t block_num) {
  size_t cur_block_num_ = AssignFreeBlock();
  size_t offset_ = 0;
  // update block info
  block_info_[cur_block_num_].status_ = PrimaryBlock;
  block_info_[cur_block_num_].corresponding_ = cur_block_num_;
  block_info_[cur_block_num_].offset_ = 0;
  // collect primary block
  for (size_t j = 0; j < PAGE_NUMS; ++ j) {
    if (page_info_[block_numn][j].status_ == PageValid) {
      size_t lba_ = page_info_[block_num][j].lba_;
      // read data
      std::pair<size_t, char*> res = ReadByPageNum(cur_block_num_, offset_);
      // write data
      WriteByPageNum(cur_block_num_, offset_, lba_, res.second);
      // update page table
      page_table_[lba_].block_num_ = cur_block_num_;
      page_table_[lba_].page_num_ = offset_;
      // update page info
      page_info_[cur_block_num_][offset_].lba_ = lba_;
      page_info_[cur_block_num_][offset_].status_ = PageValid;
      // update offset
      offset_ = offset_ + 1; 
    }
  }
  // erase primary block
  Erase(block_num);
  // collect replace block if exsits
  if (block_info_[block_num].corresponding_ != block_num) {
    block_num = block_info_[block_num].corresponding_;
    bool replaced = false;
    for (size_t j = 0; j < PAGE_NUMS; ++ j) {
      if (page_info_[block_num][j].status_ == PageValid) {
        size_t lba_ = page_info_[block_num_][j].status_;
        if (offset_ >= PAGE_NUMS) {
          // creaete replace block
          replaced = true;
          size_t replace_block_num_ = AssignFreeBlock();
          // update primary block info
          block_info_[cur_block_num_].corresponding_ = replace_block_num_;
          block_info_[cur_block_num_].offset_ = 0;
          cur_block_num_ = replace_block_num_;
          // update replace block info
          block_info_[cur_block_num_].status_ = ReplaceBlock;
          block_info_[cur_block_num_].corresponding_ = cur_block_num_;
          block_info_[cur_block_num_].offset_ = 0;
          offset_ = 0;
        }
        // read data
        std::pair<size_t, char*> res = ReadByPageNum(cur_block_num_, offset_);
        // write data
        WriteByPageNum(cur_block_num_, offset_, lba_, res.second);
        // update page table
        page_table_[lba_].block_num_ = cur_block_num_;
        page_table_[lba_].page_num_ = offset_;
        // update page info
        page_info_[cur_block_num_][offset_].lba_ = lba_;
        page_info_[cur_block_num_][offset_].status_ = PageValid;
        // update offset
        offset_ = offset_ + 1;
      }
    }
    // update replace block offset
    if (replaced)
      block_info_[cur_block_num_].offset_ = offset_;
    // erase replace block
    Erase(block_num);
  }
}

void Flash::MajorCollectGarbage() {

}

size_t Flash::AssignFreeBlock(const size_t block_num) {
  if (free_blocks_.empty()) {
    std::cout << "FREE BLOCK IS NOT ENOUGH" << std::endl;
    exit(-1);
  }
  size_t new_block_num = free_blocks_.front();
  free_blocks_.pop();
  return new_block_num;
}

}