#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>

#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <queue>
#include <stack>
#include <algorithm>

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

  char* read(int lba);

  void write(int lba, char* data);
  
private:
  const string BASE_PATH = "flashblocks/";
  const string LOG_PATH = BASE_PATH + "log.txt";
  const string TABLE_PATH = BASE_PATH + "table.txt";
  const string BLOCK_INFO_PATH = BASE_PATH + "block_info.txt";
  const int BLOCK_NUMS = 256;
  const int PAGE_NUMS = 1024;
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


  int *next_block;
  int **block_status;
  PBA *page_table;
  queue<int> free_blocks;
  vector<pair<int, int>> linked_lists;

  bool existFile(string filename) {
    fstream f(filename, ios::in);
    bool res = f.is_open();
    f.close();
    return res;
  }

  string getBlockPath(int block_num);

  void writeLog(char *l) {
    string l1(l);
    writeLog(l1);
  }

  void writeLog(string l) {
    fstream f(LOG_PATH, ios::app);
    f << l << endl;
    f.close();
  }

  void readByPageNum(int block_num, int page_num, int &lba, char *data);

  void writeByPageNum(int block_num, int page_num, int lba, char *data);

  void erase(int block_num);

  void collectGarbage();

  int assignFreeBlock();
};