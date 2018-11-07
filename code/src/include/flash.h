namespace bilsmtree {
/*
* The implement of Original NFTL
* LBA --> VBA: BLOCK_NUM = LBA / MAX_PAGE_NUM, PAGE_NUM = LBA % MAX_PAGE_NUM
* 1. Read LBA: it is traversed to find the most recent data among the 
*               replacement blocks in the linked list.
* 2. Write LBA: it is traversed to find the first free page with the same page 
*               number.
* 3. Garbage Collection: find the longest linked list and copy all valid pages 
*                         to the last replacement block.
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
  
  struct PBA {
    size_t block_num_;
    size_t page_num_;

    PBA() {
    }

    PBA(size_t a, size_t b) {
      block_num_ = a;
      page_num_ = c;
    }
  };

  struct PageInfo {
    size_t lba;
    size_t status;     // 0 free 1 valid 2 invalid

    PageInfo() {
      status = 0;
      lba = 0;
    }
  };


  std::map<size_t, PBA> page_table_;
  PageInfo **page_info_;
  size_t* block_status_;  // 0 free 1 primary 2 replacement
  size_t* corresponding_; // the corresponding primary block of replacement
  std::queue<size_t> free_blocks;

  std::string GetBlockPath(const size_t block_num) {
    char block_name[30];
    sprintf(block_name, "blocks/block_%d.txt", block_num);
    return BASE_PATH + block_name;
  }

  void WriteLog(const char *l) {
    WriteLog(std::string(l));
  }

  void WriteLog(const std::string l) {
    std::fstream f(LOG_PATH, std::ios::app | std::ios::out);
    f << time(0) << "\t" << l;
    f.close();
  }

  char* ReadByPageNum(const size_t block_num, const size_t page_num, const size_t lba);

  void WriteByPageNum(const size_t block_num, const size_t page_num, const size_t lba, const char *data);

  void Erase(const size_t block_num);

  void CollectGarbage();

  int AssignFreeBlock(const size_t block_num);

  void UpdateLinkedList(const size_t block_num);

  std::pair<size_t, size_t> FindReplacement(const size_t block_num, const size_t page_num);
};

}