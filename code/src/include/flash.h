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

class Flash {
public:
  Flash(FlashResult *flashresult);
  
  ~Flash();

  char* Read(const size_t lba);

  void Write(const size_t lba, const char* data);

  void ShowInfo();
private:  
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
    BlockStatus status_;        // 0 free 1 primary 2 replacement
    size_t next_;               // the corresponding block: 1. the replacement block 2. the primary block
    size_t prev_;               // the corresponding block whose next is this
    size_t offset_;             // the offset of replacement block    
    size_t invalid_nums_;
    size_t valid_nums_;

    BlockInfo() {
      block_num_ = 0;
      status_ = FreeBlock;
      next_ = 0;
      prev_ = 0;
      offset_ = 0;
      invalid_nums_ = 0;
      valid_nums_ = 0;
    }

    BlockInfo(size_t block_num) {
      block_num_ = block_num;
      status_ = FreeBlock;
      next_ = block_num_;
      prev_ = block_num_;
      offset_ = 0;
      invalid_nums_ = 0;
      valid_nums_ = 0;
    }

    void Show() {
      std::cout << "BLOCK:" << block_num_ << "\tSTATUS:" << status_ << std::endl;
      std::cout << "Prev:" << prev_ << "\tNext:" << next_ << "\tOffset:" << offset_ << std::endl;
      std::cout << "Invalid:" << invalid_nums_ << "\tValid:" << valid_nums_ << std::endl;
    }
  };

  std::map<size_t, PBA> page_table_;  // mapping relation of LBA(logical block address) and PBA(physical block address)
  PageInfo **page_info_;              // page infomation: LBA and page status
  BlockInfo *block_info_;             // block infomation: block status, corresponding block, block offset(invalid for replacement block)
  
  std::queue<size_t> free_blocks_;    // free blocks queue
  bool *free_block_tag_;              // free tag of block
  size_t free_blocks_num_;            // free blocks number

  FlashResult *flashresult_;

  inline std::string GetBlockPath(const size_t block_num) {
    char block_name[30];
    sprintf(block_name, "blocks/block_%zu.txt", block_num);
    return std::string(Config::FlashConfig::BASE_PATH) + block_name;
  }

  inline void WriteLog(const char *l) {
    WriteLog(std::string(l));
  }

  inline void WriteLog(const std::string l) {
    std::fstream f(Config::FlashConfig::LOG_PATH, std::ios::app | std::ios::out);
    f << time(0) << "\t" << l;
    f.close();
  }

  std::pair<size_t, char*> ReadByPageNum(const size_t block_num, const size_t page_num);

  void WriteByPageNum(const size_t block_num, const size_t page_num, const size_t lba, const char* data);

  void Erase(const size_t block_num);

  std::pair<size_t, size_t> MinorCollectGarbage(const size_t block_num);

  void MajorCollectGarbage();

  size_t AssignFreeBlock();
};

}

#endif