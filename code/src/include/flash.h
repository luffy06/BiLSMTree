#ifndef BILSMTREE_FLASH_H
#define BILSMTREE_FLASH_H

#include <iostream>
#include <queue>
#include <map>
#include <string>
#include <fstream>

namespace bilsmtree {
/*
* Page formation: DATA LBA STATUS
* Log formation: OPERATION\tLBA\tBLOCK_NUM\tPAGE_NUM\tDATA
*/
class FlashConfig {
public:
  FlashConfig();
  ~FlashConfig();

  const static double READ_WRITE_RATE = 4;
  
};

class Flash {
public:
  Flash();
  
  ~Flash();

  char* Read(const size_t lba);

  void Write(const size_t lba, const char* data);
private:
  const std::string BASE_PATH = "../logs/flashblocks/";
  const std::string BASE_LOG_PATH = "../logs/";
  const std::string LOG_PATH = BASE_LOG_PATH + "flashlog.txt";
  const std::string TABLE_PATH = BASE_PATH + "table.txt";
  const std::string NEXT_BLOCK_INFO_PATH = BASE_PATH + "nextblockinfo.txt";
  const std::string PREV_BLOCK_INFO_PATH = BASE_PATH + "prevblockinfo.txt";
  const std::string BLOCK_META_PATH = BASE_PATH + "blockmeta.txt";
  const size_t BLOCK_NUMS = 256;
  const size_t PAGE_NUMS = 8;
  const size_t PAGE_SIZE = 16 * 1024; // 16KB
  const size_t LBA_NUMS = BLOCK_NUMS * PAGE_NUMS;
  const size_t LOG_LENGTH = 1000;
  const size_t BLOCK_THRESOLD = BLOCK_NUMS * 0.8;
  
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

    PBA(size_t a, size_t b) {
      block_num_ = a;
      page_num_ = c;
    }
  };

  struct PageInfo {
    size_t lba_;
    PageStatus status_;          // 0 free 1 valid 2 invalid

    PageInfo() {
      status_ = 0;
      lba_ = PageFree;
    }
  };

  struct BlockInfo {
    BlockStatus status_;        // 0 free 1 primary 2 replacement
    size_t corresponding_;      // the corresponding primary block of replacement
    size_t offset_;             // the offset of replace block    

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
    sprintf(block_name, "blocks/block_%d.txt", block_num);
    return BASE_PATH + block_name;
  }

  inline void WriteLog(const char *l) {
    WriteLog(std::string(l));
  }

  inline void WriteLog(const std::string l) {
    std::fstream f(LOG_PATH, std::ios::app | std::ios::out);
    f << time(0) << "\t" << l;
    f.close();
  }

  std::pair<size_t, char*> Flash::ReadByPageNum(const size_t block_num, const size_t page_num);

  void WriteByPageNum(const size_t block_num, const size_t page_num, const size_t lba, const char* data);

  void Erase(const size_t block_num);

  void MinorCollectGarbage(const size_t block_num);

  void MajorCollectGarbage();

  size_t AssignFreeBlock(const size_t block_num);
};

}

#endif