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
  Flash() {
    fstream f;
    // create block files
    for (int i = 0; i < BLOCK_NUMS; ++ i) {
      string block_path = getBlockPath(i);
      if (!existFile(block_path)) {
        f.open(block_path, ios::app | ios::out);
        f.close();
      }
    }
    
    // load block meta files
    block_status = new int*[BLOCK_NUMS];
    for (int i = 0; i < BLOCK_NUMS; ++ i)
      block_status[i] = new int[PAGE_NUMS];
    for (int i = 0; i < BLOCK_NUMS; ++ i)
      for (int j = 0; j < PAGE_NUMS; ++ j)
        block_status[i][j] = 0;
    string block_meta_path = BASE_PATH + "block_meta.txt";
    if (existFile(block_meta_path)) {
      cout << "block meta exist" << endl;
      f.open(block_meta_path, ios::in);
      while (!f.eof()) {
        int i, j, status;
        f.read((char *)&i, sizeof(int));
        f.read((char *)&j, sizeof(int));
        f.read((char *)&status, sizeof(int));
        block_status[i][j] = status;
      }
      f.close();
    }

    // load table info
    page_table = new PBA[LBA_NUMS];
    if (existFile(TABLE_PATH)) {
      cout << "table info exist" << endl;
      f.open(TABLE_PATH, ios::in);
      int lba, bn, pn;
      while (!f.eof()) {
        f.read((char *)&lba, sizeof(int));
        f.read((char *)&bn, sizeof(int));
        f.read((char *)&pn, sizeof(int));
        if (lba >= 0 && lba < LBA_NUMS && 
          bn >= 0 && bn < BLOCK_NUMS && 
          pn >= 0 && pn < PAGE_NUMS)
          page_table[lba].setData(bn, pn);
      }
      f.close();
    }

    // load next block info
    next_block = new int[BLOCK_NUMS];
    for (int i = 0; i < BLOCK_NUMS; ++ i)
      next_block[i] = i;
    if (existFile(BLOCK_INFO_PATH)) {
      cout << "block info exist" << endl;
      f.open(BLOCK_INFO_PATH, ios::in);
      int from, to;
      while (!f.eof()) {
        f.read((char *)&from, sizeof(int));
        f.read((char *)&to, sizeof(int));
        next_block[from] = to; 
      }
      f.close();
    }

    // load free blocks
    for (int i = 0; i < BLOCK_NUMS; ++ i) {
      bool is_free = true;
      for (int j = 0; j < PAGE_NUMS; ++ j)
        if (block_status[i][j] != 0)
          is_free = false;
      if (is_free)
        free_blocks.push(i);
    }
  }
  
  ~Flash() {
    // write block status
    fstream f;
    string block_meta_path = BASE_PATH + "block_meta.txt";
    f.open(block_meta_path, ios::ate | ios::out);
    for (int i = 0; i < BLOCK_NUMS; ++ i) {
      for (int j = 0; j < PAGE_NUMS; ++ j) {
        if (block_status[i][j] != 0) {
          f.write((char *)&i, sizeof(int));
          f.write((char *)&j, sizeof(int));
          f.write((char *)&block_status[i][j], sizeof(int));
        }
      }
    }
    f.close();
    // destory block_status
    for (int i = 0; i < BLOCK_NUMS; ++ i)
      delete[] block_status[i];
    delete[] block_status;

    // write page table
    f.open(TABLE_PATH, ios::ate | ios::out);
    for (int i = 0; i < LBA_NUMS; ++ i) {
      if (page_table[i].used) {
        f.write((char *)&i, sizeof(int));
        f.write((char *)&page_table[i].block_num, sizeof(int));
        f.write((char *)&page_table[i].page_num, sizeof(int));
      }
    }
    f.close();
    // destory page_table
    delete[] page_table;

    // write block info
    f.open(BLOCK_INFO_PATH, ios::ate | ios::out);
    for (int i = 0; i < BLOCK_NUMS; ++ i) {
      if (next_block[i] != i) {
        f.write((char *)&i, sizeof(int));
        f.write((char *)&next_block[i], sizeof(int));
      }
    }
    f.close();
    // destory next_block
    delete[] next_block; 
  }

  char* read(int lba) {
    // calculate block num and page num
    int block_num = lba / PAGE_NUMS;
    int page_num = lba % PAGE_NUMS;
    if (page_table[lba].used) {
      block_num = page_table[lba].block_num;
      page_num = page_table[lba].page_num;
    }
    // read page
    int _lba;
    char *data = new char[PAGE_SIZE];
    readByPageNum(block_num, page_num, _lba, data);
    return data;
  }

  void write(int lba, char* data) {
    // calculate block num and page num
    int block_num = lba / PAGE_NUMS;
    int page_num = lba % PAGE_NUMS;
    while (true) {
      if (block_status[block_num][page_num] == 0)
        break;
      int _lba;
      char *buffer = new char[PAGE_SIZE];
      readByPageNum(block_num, page_num, _lba, buffer);
      if (_lba == lba)
        block_status[block_num][page_num] = 2;

      if (next_block[block_num] == block_num) {
        next_block[block_num] = assignFreeBlock();
      }
      else {
        block_num = next_block[block_num];
      }
    }
    // write page
    writeByPageNum(block_num, page_num, lba, data);
    // write page table
    page_table[lba].setData(block_num, page_num);
    // check whether start garbage collection
    if (BLOCK_NUMS - free_blocks.size() > BLOCK_THRESOLD) 
      collectGarbage();
  }
  
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
    bool res = false;
    if (f.is_open()) res = true;
    f.close();
    return res;
  }

  string getBlockPath(int block_num) {
    char block_name[30];
    sprintf(block_name, "block_%d.txt", block_num);
    return BASE_PATH + block_name;
  }

  void writeLog(char *l) {
    string l1(l);
    writeLog(l1);
  }

  void writeLog(string l) {
    fstream f(LOG_PATH, ios::app);
    f << l << endl;
    f.close();
  }

  void readByPageNum(int block_num, int page_num, int &lba, char *data) {
    fstream f(getBlockPath(block_num), ios::in);
    f.seekp(page_num * (PAGE_SIZE + sizeof(int)));
    f.read(data, PAGE_SIZE * sizeof(char));
    f.read((char *)&lba, sizeof(int));
    f.close();
    // write log
    char log[LOG_LENGTH];
    sprintf(log, "READ\t%d\t%d\t%d\t%s\n", lba, block_num, page_num, data);
    writeLog(log);
  }

  void writeByPageNum(int block_num, int page_num, int lba, char *data) {
    fstream f(getBlockPath(block_num), ios::out);
    f.seekp(page_num * (PAGE_SIZE + sizeof(int)));
    f.write(data, PAGE_SIZE * sizeof(char));
    f.write((char *)&lba, sizeof(int));
    f.close();
    // write log
    char log[LOG_LENGTH];
    sprintf(log, "WRITE\t%d\t%d\t%d\t%s\n", lba, block_num, page_num, data);
    writeLog(log);
  }

  void erase(int block_num) {
    // clean block data
    fstream f(getBlockPath(block_num), ios::ate | ios:: out);
    f.close();
    // clean block status
    for (int j = 0; j < PAGE_NUMS; ++ j)
      block_status[block_num][j] = 0;
    // insert into free queue
    free_blocks.push(block_num);
    // update next block
    next_block[block_num] = block_num;
    // write log
    char log[LOG_LENGTH];
    sprintf(log, "ERASE\t%d\n", block_num);
    writeLog(log);
  }

  void collectGarbage() {
    // find max length linked list
    int head = linked_lists[0].first;
    int max_len = linked_lists[0].second;
    int index = 0;
    for (int i = 1; i < linked_lists.size(); ++ i) {
      if (linked_lists[i].second > max_len) {
        head = linked_lists[i].first;
        max_len = linked_lists[i].second;
        index = i;
      }
    }
    linked_lists.erase(linked_lists.begin() + index);
    // find last replacement block
    int last = head;
    while (next_block[last] != last)
      last = next_block[last];
    int page_index = 0; // page index of last block
    for (int i = head; next_block[i] != last; i = next_block[i]) {
      for (int j = 0; j < PAGE_NUMS; ++ j) {
        if (block_status[i][j] == 1) {
          while (page_index < PAGE_NUMS && block_status[last][page_index] != 0)
            page_index = page_index + 1;
          if (page_index == PAGE_NUMS) {
            page_index = 0;
            last = assignFreeBlock();
          }
          // read old page
          int lba;
          char *data = new char[PAGE_SIZE];
          readByPageNum(i, j, lba, data);
          // write new page
          writeByPageNum(last, page_index, lba, data);
          // update page table
          page_table[lba].setData(last, page_index);
        }
        // update block status
        block_status[i][j] = 0;
      }
      // erase block
      erase(i);
    }
  }

  int assignFreeBlock() {
    if (!free_blocks.empty()) 
      exit(-1);
    int block_num = free_blocks.front();
    free_blocks.pop();
    return block_num;
  }
};

int main() {
  Flash flash = Flash();

  return 0;
}