#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <vector>

using namespace std;

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
class Flash {
public:
  Flash();
  
  ~Flash();

  char* read(const int lba);

  void write(const int lba, const char* data);
  
private:
  const string BASE_PATH = "../flashblocks/";
  const string BASE_LOG_PATH = "../logs/";
  const string LOG_PATH = BASE_LOG_PATH + "flashlog.txt";
  const string TABLE_PATH = BASE_PATH + "table.txt";
  const string BLOCK_INFO_PATH = BASE_PATH + "blockinfo.txt";
  const string BLOCK_META_PATH = BASE_PATH + "blockmeta.txt";
  const int BLOCK_NUMS = 256;
  const int PAGE_NUMS = 8;
  const int PAGE_SIZE = 16 * 1024 * 8; // 16KB
  const int LBA_NUMS = BLOCK_NUMS * PAGE_NUMS;
  const int LOG_LENGTH = 1000;
  const int BLOCK_THRESOLD = BLOCK_NUMS * 0.1;
  
  struct PBA {
    int block_num;
    int page_num;
    bool used;

    PBA() {
      used = false;
    }

    void setData(int bn, int pn) {
      block_num = bn;
      page_num = pn;
      used = true;
    }
  };

  struct BlockInfo {
    int lba;
    int status;

    BlockInfo() {
      status = 0;
      lba = -1;
    }
  };


  int *next_block;
  BlockInfo **block_info;
  PBA *page_table;
  queue<int> free_blocks;
  bool *free_block_tag;
  int free_blocks_num;
  vector<pair<int, int>> linked_lists;

  bool existFile(const string filename) {
    fstream f(filename, ios::in);
    bool res = f.is_open();
    f.close();
    return res;
  }

  string getBlockPath(const int block_num) {
    char block_name[30];
    sprintf(block_name, "blocks/block_%d.txt", block_num);
    return BASE_PATH + block_name;
  }

  void writeLog(const char *l) {
    string l1(l);
    writeLog(l1);
  }

  void writeLog(const string l) {
    fstream f(LOG_PATH, ios::app | ios::out);
    f << time(0) << "\t" << l;
    f.close();
  }

  void readByPageNum(const int block_num, const int page_num, const int &lba, char *data);

  void writeByPageNum(const int block_num, const int page_num, const int lba, const char *data);

  void erase(const int block_num);

  void collectGarbage();

  int assignFreeBlock();
};