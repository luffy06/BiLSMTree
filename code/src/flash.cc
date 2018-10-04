#include "flash.h"

Flash::Flash() {
  std::cout << "CREATING FLASH" << std::endl;
  std::fstream f;
  // create block files
  for (int i = 0; i < BLOCK_NUMS; ++ i) {
    std::string block_path = GetBlockPath(i);
    if (!Util::ExistFile(block_path)) {
      std::cout << "CREATING BLOCK FILE " << i << std::endl;
      f.open(block_path, std::ios::app | std::ios::out);
      f.close();
    }
  }
  
  // load block meta files
  block_info = new BlockInfo*[BLOCK_NUMS];
  for (int i = 0; i < BLOCK_NUMS; ++ i)
    block_info[i] = new BlockInfo[PAGE_NUMS];
  if (Util::ExistFile(BLOCK_META_PATH)) {
    std::cout << "LOADING BLOCK META" << std::endl;
    f.open(BLOCK_META_PATH, std::ios::in);
    if (!f.is_open()) {
      std::cout << "OPEN BLOCK META FILE FAILED" << std::endl;
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
  if (Util::ExistFile(TABLE_PATH)) {
    std::cout << "LOADING PAGE TABLE" << std::endl;
    f.open(TABLE_PATH, std::ios::in);
    if (!f.is_open()) {
      std::cout << "OPEN PAGE TABLE FILE FAILED" << std::endl;
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
        page_table[lba].SetData(bn, pn);
    }
    f.close();
  }

  // load next block info
  next_block = new int[BLOCK_NUMS];
  for (int i = 0; i < BLOCK_NUMS; ++ i)
    next_block[i] = i;
  if (Util::ExistFile(NEXT_BLOCK_INFO_PATH)) {
    std::cout << "LOADING NEXT BLOCK INFO" << std::endl;
    f.open(NEXT_BLOCK_INFO_PATH, std::ios::in);
    if (!f.is_open()) {
      std::cout << "OPEN NEXT BLOCK INFO FILE FAILED" << std::endl;
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

  // load prev block info
  prev_block = new int[BLOCK_NUMS];
  for (int i = 0; i < BLOCK_NUMS; ++ i)
    prev_block[i] = i;
  if (Util::ExistFile(PREV_BLOCK_INFO_PATH)) {
    std::cout << "LOADING PREV BLOCK INFO" << std::endl;
    f.open(PREV_BLOCK_INFO_PATH, std::ios::in);
    if (!f.is_open()) {
      std::cout << "OPEN PREV BLOCK INFO FILE FAILED" << std::endl;
      exit(-1);
    }
    int from, to;
    while (!f.eof()) {
      f.read((char *)&from, sizeof(int));
      f.read((char *)&to, sizeof(int));
      prev_block[from] = to; 
    }
    f.close();
  }

  // load free blocks
  std::cout << "LOADING FREE BLOCKS" << std::endl;
  free_blocks_num = 0;
  free_block_tag = new bool[BLOCK_NUMS];
  for (int i = 0; i < BLOCK_NUMS; ++ i) {
    free_block_tag[i] = false;
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
  std::cout << "FREE BLOCKS'S NUMBER:" << free_blocks_num << std::endl;

  std::cout << "CREATE SUCCESS" << std::endl;
}
  
Flash::~Flash() {
  if (Config::PERSISTENCE) {
    std::fstream f;
    // write block status
    std::cout << "UNLOADING BLOCK META" << std::endl;
    f.open(BLOCK_META_PATH, std::ios::ate | std::ios::out);
    if (!f.is_open()) {
      std::cout << "OPEN BLOCK META FILE FAILED" << std::endl;
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
    // write page table
    std::cout << "UNLOADING PAGE TABLE" << std::endl;
    f.open(TABLE_PATH, std::ios::ate | std::ios::out);
    if (!f.is_open()) {
      std::cout << "OPEN PAGE TABLE FILE FAILED" << std::endl;
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
    // write block info
    std::cout << "UNLOADING NEXT BLOCK INFO" << std::endl;
    f.open(NEXT_BLOCK_INFO_PATH, std::ios::ate | std::ios::out);
    if (!f.is_open()) {
      std::cout << "OPEN NEXT BLOCK INFO FILE FAILED" << std::endl;
      exit(-1);
    }
    for (int i = 0; i < BLOCK_NUMS; ++ i) {
      if (next_block[i] != i) {
        f.write((char *)&i, sizeof(int));
        f.write((char *)&next_block[i], sizeof(int));
      }
    }
    f.close();

    // write block info
    std::cout << "UNLOADING PREV BLOCK INFO" << std::endl;
    f.open(PREV_BLOCK_INFO_PATH, std::ios::ate | std::ios::out);
    if (!f.is_open()) {
      std::cout << "OPEN PREV BLOCK INFO FILE FAILED" << std::endl;
      exit(-1);
    }
    for (int i = 0; i < BLOCK_NUMS; ++ i) {
      if (prev_block[i] != i) {
        f.write((char *)&i, sizeof(int));
        f.write((char *)&prev_block[i], sizeof(int));
      }
    }
    f.close();
  }

  // destory block_info
  for (int i = 0; i < BLOCK_NUMS; ++ i)
    delete[] block_info[i];
  delete[] block_info;

  // destory page_table
  delete[] page_table;

  // destory next_block and prev_block
  delete[] next_block;
  delete[] prev_block;

  delete[] free_block_tag;
}

char* Flash::Read(const int lba) {
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
  ReadByPageNum(block_num, page_num, _lba, data);
  return data;
}

void Flash::Write(const int lba, const char* data) {
  // std::cout << "WRITE LBA:" << lba << std::endl;
  // calculate block num and page num
  int block_num = lba / PAGE_NUMS;
  int page_num = lba % PAGE_NUMS;
  while (block_info[block_num][page_num].status != 0) {
    if (block_info[block_num][page_num].lba == lba)
      block_info[block_num][page_num].status = 2;

    if (next_block[block_num] == block_num) {
      AssignFreeBlock(block_num);
    }
    block_num = next_block[block_num];
  }
  // write page
  WriteByPageNum(block_num, page_num, lba, data);
  // update block info
  block_info[block_num][page_num].status = 1;
  block_info[block_num][page_num].lba = lba;
  if (free_block_tag[block_num]) {
    free_block_tag[block_num] = false;
    free_blocks_num = free_blocks_num - 1;
  }
  // write page table
  page_table[lba].SetData(block_num, page_num);
  // std::cout << "FREE BLOCKS IN FLASH:" << free_blocks_num << std::endl;
  // check whether start garbage collection
  if (free_blocks_num < BLOCK_THRESOLD) 
    CollectGarbage();
}

void Flash::ReadByPageNum(const int block_num, const int page_num, const int &lba, char *data) {
  std::fstream f(GetBlockPath(block_num), std::ios::in);
  f.seekp(page_num * (PAGE_SIZE * sizeof(char) + sizeof(int)));
  f.read(data, PAGE_SIZE * sizeof(char));
  f.read((char *)&lba, sizeof(int));
  f.close();
  data[PAGE_SIZE] = '\0';
  // write log
  char log[LOG_LENGTH];
  sprintf(log, "READ\t%d\t%d\t%d\t%s\n", lba, block_num, page_num, data);
  WriteLog(log);
}

void Flash::WriteByPageNum(const int block_num, const int page_num, const int lba, const char *data) {
  // std::cout << "WRITE IN FLASH BLOCK NUM:" << block_num << " PAGE NUM:" << page_num << std::endl;
  char *ndata = new char[PAGE_SIZE];
  strcpy(ndata, data);
  std::fstream f(GetBlockPath(block_num), std::ios::out | std::ios::in);
  f.seekp(page_num * (PAGE_SIZE * sizeof(char) + sizeof(int)));
  f.write(ndata, PAGE_SIZE * sizeof(char));
  f.write((char *)&lba, sizeof(int));
  f.close();
  // write log
  char log[LOG_LENGTH];
  sprintf(log, "WRITE\t%d\t%d\t%d\t%s\n", lba, block_num, page_num, data);
  WriteLog(log);
}

void Flash::Erase(const int block_num) {
  // std::cout  << "ERASE " << block_num << std::endl;
  // clean block data
  std::fstream f(GetBlockPath(block_num), std::ios::ate | std::ios:: out);
  f.close();
  // clean block status
  for (int j = 0; j < PAGE_NUMS; ++ j)
    block_info[block_num][j] = BlockInfo();
  // insert into free queue
  free_blocks.push(block_num);
  free_blocks_num = free_blocks_num + 1;
  free_block_tag[block_num] = true;
  // update next and prev block
  next_block[block_num] = block_num;
  prev_block[block_num] = block_num;
  // write log
  char log[LOG_LENGTH];
  sprintf(log, "ERASE\t%d\n", block_num);
  WriteLog(log);
}

void Flash::CollectGarbage() {
  // std::cout << "START GARBAGE COLLECTION\tFREE BLOCK NUM:" << free_blocks_num << std::endl;
  // find max length linked list
  int head = -1;
  int max_len = 0;
  // TODO: LOW EFFCIENCY
  for (int i = 0; i < BLOCK_NUMS; ++ i) {
    int ct = 1;
    for (int j = i; j != next_block[j]; j = next_block[j], ++ ct)
      ;
    if (ct > max_len) {
      head = i;
      max_len = ct;
    }
  }
  // std::cout << "HEAD:" << head << " MAX LEN:" << max_len << std::endl;
  // find last replacement block
  int last = head;
  while (next_block[last] != last)
    last = next_block[last];
  int page_index = 0; // page index of last block
  int now_block = last;
  for (int i = head; i != last; ) {
    for (int j = 0; j < PAGE_NUMS; ++ j) {
      if (block_info[i][j].status == 1) {
        while (page_index < PAGE_NUMS && block_info[now_block][page_index].status != 0)
          page_index = page_index + 1;
        if (page_index == PAGE_NUMS) {
          page_index = 0;
          AssignFreeBlock(now_block);
          now_block = next_block[now_block];
        }
        // read old page
        int lba;
        char *data = new char[PAGE_SIZE + 1];
        ReadByPageNum(i, j, lba, data);
        // write new page
        WriteByPageNum(now_block, page_index, lba, data);
        
        block_info[now_block][page_index].status = 1;
        block_info[now_block][page_index].lba = lba;
        if (free_block_tag[now_block]) {
          free_block_tag[now_block] = false;
          free_blocks_num = free_blocks_num - 1;
        }
        // update page table
        page_table[lba].SetData(now_block, page_index);
      }
      // update block status
      block_info[i][j] = BlockInfo();
    }
    int erase_block = i;
    i = next_block[i];
    // erase block
    Erase(erase_block);
  }
  // std::cout << "END GARBAGE COLLECTION\tFREE BLOCK NUM:" << free_blocks_num << std::endl;
}

int Flash::AssignFreeBlock(const int &block_num) {
  // std::cout << "ASSIGN FREE BLOCK" << std::endl;
  if (free_blocks.empty()) {
    std::cout << "FREE BLOCK IS NOT ENOUGH" << std::endl;
    exit(-1);
  }
  int new_block_num = free_blocks.front();
  free_blocks.pop();
  // find free blocks
  while (!free_blocks.empty() && !free_block_tag[new_block_num]) {
    new_block_num = free_blocks.front();
    free_blocks.pop();
  }
  // set next_block
  if (block_num != -1) {
    next_block[block_num] = new_block_num;
    prev_block[new_block_num] = block_num;
  }
  return new_block_num;
}