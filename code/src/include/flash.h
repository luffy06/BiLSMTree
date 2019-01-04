#ifndef BILSMTREE_FLASH_H
#define BILSMTREE_FLASH_H

#include <iostream>
#include <queue>
#include <map>
#include <set>
#include <string>
#include <fstream>

#include "util.h"

namespace bilsmtree {
/*
* Page formation: DATA LBA STATUS
* Log formation: OPERATION\tLBA\tBLOCK_NUM\tPAGE_NUM\tDATA
*/

enum PageStatus {
  PageFree,
  PageValid,
  PageInvalid
};

enum BlockStatus {
  FreeBlock,
  PrimaryBlock,
  ReplaceBlock
};

struct PBA {
  size_t block_num_;
  size_t page_num_;

  PBA() {
    block_num_ = 0;
    page_num_ = 0;      
  }

  PBA(size_t a, size_t b) {
    block_num_ = a;
    page_num_ = b;
  }
};

struct PageInfo {
  size_t lba_;
  PageStatus status_;          // 0 free 1 valid 2 invalid

  PageInfo() {
    status_ = PageFree;
    lba_ = 0;
  }
};

struct BlockInfo {
  size_t block_num_;
  int father_;
  int left_;
  int right_;
  size_t offset_;
  size_t invalid_nums_;
  size_t valid_nums_;

  BlockInfo() {
    block_num_ = 0;
    father_ = -1;
    left_ = -1;
    right_ = -1;
    offset_ = 0;
    invalid_nums_ = 0;
    valid_nums_ = 0;
  }

  BlockInfo(size_t block_num, size_t father, size_t left, size_t right) {
    block_num_ = block_num;
    father_ = father;
    left_ = left;
    right_ = right;
    offset_ = 0;
    invalid_nums_ = 0;
    valid_nums_ = 0;
  }

  void Show() {
    std::cout << "BLOCK:" << block_num_ << "\tFree:" << Config::FlashConfig::PAGE_NUMS - invalid_nums_ - valid_nums_ << std::endl;
    std::cout << "Left:" << left_ << "\tRight:" << right_ << "\tOffset:" << offset_ << std::endl;
    std::cout << "Invalid:" << invalid_nums_ << "\tValid:" << valid_nums_ << std::endl;
  }
};

class Flash {
public:
  Flash(FlashResult *flashresult);
  
  ~Flash();

  char* Read(const size_t lba);

  void Write(const size_t lba, const char* data);

  void Invalidate(const size_t lba);

  void ShowInfo();
private:  
  std::map<size_t, PBA> page_table_;              // mapping relation of LBA(logical block address) and PBA(physical block address)
  std::vector<std::vector<PageInfo>> page_info_;  // page infomation: LBA and page status
  std::vector<BlockInfo> block_info_;             // block infomation: block status, corresponding block, block offset(invalid for replacement block)
  
  size_t free_pages_num_;                         // free blocks number
  size_t heap_;

  FlashResult *flashresult_;

  inline std::string GetBlockPath(const size_t block_num) {
    char block_name[30];
    sprintf(block_name, "blocks/block_%zu.txt", block_num);
    return std::string(Config::FlashConfig::BASE_PATH) + block_name;
  }

  std::pair<size_t, char*> ReadByPageNum(const size_t block_num, const size_t page_num);

  void WriteByPageNum(const size_t block_num, const size_t page_num, const size_t lba, const char* data);

  void Erase(const size_t block_num);

  void MajorCollectGarbage();

  PBA GetLocation();

  void AdjustHeap(int block_num);

  int GetFreePageNumber(int block_num);
};

}

#endif