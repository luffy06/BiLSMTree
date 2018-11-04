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

  char* Read(const int lba);

  void Write(const int lba, const char* data);
  
private:
  const std::string BASE_PATH = "../logs/flashblocks/";
  const std::string BASE_LOG_PATH = "../logs/";
  const std::string LOG_PATH = BASE_LOG_PATH + "flashlog.txt";
  const std::string TABLE_PATH = BASE_PATH + "table.txt";
  const std::string NEXT_BLOCK_INFO_PATH = BASE_PATH + "nextblockinfo.txt";
  const std::string PREV_BLOCK_INFO_PATH = BASE_PATH + "prevblockinfo.txt";
  const std::string BLOCK_META_PATH = BASE_PATH + "blockmeta.txt";
  const int BLOCK_NUMS = 256;
  const int PAGE_NUMS = 8;
  const int PAGE_SIZE = 16 * 1024; // 16KB
  const int LBA_NUMS = BLOCK_NUMS * PAGE_NUMS;
  const int LOG_LENGTH = 1000;
  const int BLOCK_THRESOLD = BLOCK_NUMS * 0.8;
  
  struct PBA {
    int block_num;
    int page_num;
    bool used;

    PBA() {
      used = false;
    }

    void SetData(int bn, int pn) {
      block_num = bn;
      page_num = pn;
      used = true;
    }
  };

  struct BlockInfo {
    int lba;
    int status; // 0 free 1 valid 2 invalid

    BlockInfo() {
      status = 0;
      lba = -1;
    }
  };


  int *next_block;
  int *prev_block;
  BlockInfo **block_info;
  PBA *page_table;
  std::queue<int> free_blocks;
  bool *free_block_tag; // tag those blocks in `free_blocks` queue whether free or not
  int free_blocks_num;

  std::string GetBlockPath(const int block_num) {
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

  void ReadByPageNum(const int block_num, const int page_num, const int &lba, char *data);

  void WriteByPageNum(const int block_num, const int page_num, const int lba, const char *data);

  void Erase(const int block_num);

  void CollectGarbage();

  int AssignFreeBlock(const int &block_num);

  void UpdateLinkedList(const int &block_num);
};

}