#include "flash.h"
Flash::Flash() {
  cout << "CREATING FLASH" << endl;
  fstream f;
  // create block files
  for (int i = 0; i < BLOCK_NUMS; ++ i) {
    string block_path = getBlockPath(i);
    if (!existFile(block_path)) {
      cout << "CREATING BLOCK FILE " << i << endl;
      f.open(block_path, ios::app | ios::out);
      f.close();
    }
  }
  
  // load block meta files
  block_info = new BlockInfo*[BLOCK_NUMS];
  for (int i = 0; i < BLOCK_NUMS; ++ i)
    block_info[i] = new BlockInfo[PAGE_NUMS];
  if (existFile(BLOCK_META_PATH)) {
    cout << "LOADING BLOCK META" << endl;
    f.open(BLOCK_META_PATH, ios::in);
    if (!f.is_open()) {
      cout << "OPEN BLOCK META FILE FAILED" << endl;
      exit(-1);
    }
    while (!f.eof()) {
      int i, j;
      BlockInfo bs;
      f.read((char *)&i, sizeof(int));
      f.read((char *)&j, sizeof(int));
      f.read((char *)&bs, sizeof(BlockInfo));
      block_info[i][j] = bs;
    }
    f.close();
  }

  // load table info
  page_table = new PBA[LBA_NUMS];
  if (existFile(TABLE_PATH)) {
    cout << "LOADING PAGE TABLE" << endl;
    f.open(TABLE_PATH, ios::in);
    if (!f.is_open()) {
      cout << "OPEN PAGE TABLE FILE FAILED" << endl;
      exit(-1);
    }
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
    cout << "LOADING BLOCK INFO" << endl;
    f.open(BLOCK_INFO_PATH, ios::in);
    if (!f.is_open()) {
      cout << "OPEN BLOCK INFO FILE FAILED" << endl;
      exit(-1);
    }
    int from, to;
    while (!f.eof()) {
      f.read((char *)&from, sizeof(int));
      f.read((char *)&to, sizeof(int));
      next_block[from] = to; 
    }
    f.close();
  }

  // load free blocks
  cout << "LOADING FREE BLOCKS" << endl;
  free_blocks_num = 0;
  free_block_tag = new bool[BLOCK_NUMS];
  memset(free_block_tag, false, sizeof(bool) * BLOCK_NUMS);
  for (int i = 0; i < BLOCK_NUMS; ++ i) {
    bool is_free = true;
    for (int j = 0; j < PAGE_NUMS; ++ j)
      if (block_info[i][j].status != 0)
        is_free = false;
    if (is_free) {
      // cout << "BLOCK " << i << " IS FREE" << endl;
      free_blocks.push(i);
      free_block_tag[i] = true;
      free_blocks_num = free_blocks_num + 1;
    }
  }
  cout << "FREE BLOCKS'S NUMBER:" << free_blocks_num << endl;
  cout << "CREATE SUCCESS" << endl;
}
  
Flash::~Flash() {
  fstream f;
  // write block status
  cout << "UNLOADING BLOCK META" << endl;
  f.open(BLOCK_META_PATH, ios::ate | ios::out);
  if (!f.is_open()) {
    cout << "OPEN BLOCK META FILE FAILED" << endl;
    exit(-1);
  }
  for (int i = 0; i < BLOCK_NUMS; ++ i) {
    for (int j = 0; j < PAGE_NUMS; ++ j) {
      if (block_info[i][j].status != 0) {
        f.write((char *)&i, sizeof(int));
        f.write((char *)&j, sizeof(int));
        f.write((char *)&block_info[i][j], sizeof(BlockInfo));
      }
    }
  }
  f.close();
  // destory block_info
  for (int i = 0; i < BLOCK_NUMS; ++ i)
    delete[] block_info[i];
  delete[] block_info;

  // write page table
  cout << "UNLOADING PAGE TABLE" << endl;
  f.open(TABLE_PATH, ios::ate | ios::out);
  if (!f.is_open()) {
    cout << "OPEN PAGE TABLE FILE FAILED" << endl;
    exit(-1);
  }
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
  cout << "UNLOADING BLOCK INFO" << endl;
  f.open(BLOCK_INFO_PATH, ios::ate | ios::out);
  if (!f.is_open()) {
    cout << "OPEN BLOCK INFO FILE FAILED" << endl;
    exit(-1);
  }
  for (int i = 0; i < BLOCK_NUMS; ++ i) {
    if (next_block[i] != i) {
      f.write((char *)&i, sizeof(int));
      f.write((char *)&next_block[i], sizeof(int));
    }
  }
  f.close();
  // destory next_block
  delete[] next_block; 

  delete[] free_block_tag;
}

char* Flash::read(const int lba) {
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

void Flash::write(const int lba, const char* data) {
  // calculate block num and page num
  int block_num = lba / PAGE_NUMS;
  int page_num = lba % PAGE_NUMS;
  while (true) {
    if (block_info[block_num][page_num].status == 0)
      break;    
    if (block_info[block_num][page_num].lba == lba)
      block_info[block_num][page_num].status = 2;

    if (next_block[block_num] == block_num) {
      next_block[block_num] = assignFreeBlock();
    }
    block_num = next_block[block_num];
  }
  // write page
  writeByPageNum(block_num, page_num, lba, data);
  // update block info
  block_info[block_num][page_num].status = 1;
  block_info[block_num][page_num].lba = lba;
  if (free_block_tag[block_num] == true) {
    free_block_tag[block_num] = false;
    free_blocks_num = free_blocks_num - 1;
  }
  // write page table
  page_table[lba].setData(block_num, page_num);
  // check whether start garbage collection
  if (BLOCK_NUMS - free_blocks_num > BLOCK_THRESOLD) 
    collectGarbage();
}

void Flash::readByPageNum(const int block_num, const int page_num, const int &lba, char *data) {
  fstream f(getBlockPath(block_num), ios::in);
  f.seekp(page_num * (PAGE_SIZE * sizeof(char) + sizeof(int)));
  f.read(data, PAGE_SIZE * sizeof(char));
  f.read((char *)&lba, sizeof(int));
  f.close();
  // write log
  char log[LOG_LENGTH];
  sprintf(log, "READ\t%d\t%d\t%d\t%s\n", lba, block_num, page_num, data);
  writeLog(log);
}

void Flash::writeByPageNum(const int block_num, const int page_num, const int lba, const char *data) {
  char *ndata = new char[PAGE_SIZE];
  strcpy(ndata, data);
  fstream f(getBlockPath(block_num), ios::out | ios::in);
  f.seekp(page_num * (PAGE_SIZE * sizeof(char) + sizeof(int)));
  f.write(ndata, PAGE_SIZE * sizeof(char));
  f.write((char *)&lba, sizeof(int));
  f.close();
  // write log
  char log[LOG_LENGTH];
  sprintf(log, "WRITE\t%d\t%d\t%d\t%s\n", lba, block_num, page_num, data);
  writeLog(log);
}

void Flash::erase(const int block_num) {
  // clean block data
  fstream f(getBlockPath(block_num), ios::ate | ios:: out);
  f.close();
  // clean block status
  for (int j = 0; j < PAGE_NUMS; ++ j)
    block_info[block_num][j] = BlockInfo();
  // insert into free queue
  free_blocks.push(block_num);
  // update next block
  next_block[block_num] = block_num;
  // write log
  char log[LOG_LENGTH];
  sprintf(log, "ERASE\t%d\n", block_num);
  writeLog(log);
}

void Flash::collectGarbage() {
  cout << "START GARBAGE COLLECTION" << endl;
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
      if (block_info[i][j].status == 1) {
        while (page_index < PAGE_NUMS && block_info[last][page_index].status != 0)
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
      block_info[i][j] = BlockInfo();
    }
    // erase block
    erase(i);
  }
}

int Flash::assignFreeBlock() {
  cout << "ASSIGN FREE BLOCK" << endl;
  if (free_blocks.empty()) {
    cout << "FREE BLOCK IS NOT ENOUGH" << endl;
    exit(-1);
  }
  int block_num = free_blocks.front();
  free_blocks.pop();
  if (!free_block_tag[block_num]) {
    while (!free_blocks.empty() && !free_block_tag[block_num]) {
      block_num = free_blocks.front();
      free_blocks.pop();
    }
  }
  return block_num;
}

int main() {
  Flash flash = Flash();

  vector<int> lba_list = {17, 17, 17, 19, 20, 20, 20, 20};
  vector<string> content = {"A", "A1", "A2", "B", "C", "C1", "C2", "C3"};
  for (int i = 0; i < lba_list.size(); ++ i) {
    int lba = lba_list[i];
    string data = content[i];
    cout << "WRITE LBA:" << lba << " CONTENT:" << data << endl;
    flash.write(lba, data.c_str());
  }
  char *d = flash.read(17);
  cout << "LBA:" << 17 << " CONTENT:" << d << endl; 
  d = flash.read(20);
  cout << "LBA:" << 20 << " CONTENT:" << d << endl;   
  return 0;
}