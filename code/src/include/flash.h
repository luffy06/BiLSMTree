#ifndef BILSMTREE_FLASH_H
#define BILSMTREE_FLASH_H

#include <iostream>
#include <queue>
#include <map>
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
  Flash();
  
  ~Flash();

  char* Read(const size_t lba);

  void Write(const size_t lba, const char* data);
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
    BlockStatus status_;        // 0 free 1 primary 2 replacement
    size_t corresponding_;      // the corresponding primary block of replacement
    size_t offset_;             // the offset of replace block    

    BlockInfo() {
      status_ = FreeBlock;
      corresponding_ = 0;
      offset_ = 0;
    }

    BlockInfo(size_t block_num) {
      status_ = FreeBlock;
      corresponding_ = block_num;
      offset_ = 0;
    }
  };

  std::map<size_t, PBA> page_table_;
  PageInfo **page_info_;
  BlockInfo *block_info_;
  std::queue<size_t> free_blocks_;

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