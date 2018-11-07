namespace bilsmtree {

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
  
  page_info_ = new PageInfo*[BLOCK_NUMS];
  for (int i = 0; i < BLOCK_NUMS; ++ i)
    page_info_[i] = new PageInfo[PAGE_NUMS];
  page_table = new PBA[LBA_NUMS];
  
  free_blocks_num = 0;
  free_block_tag = new bool[BLOCK_NUMS];
  for (int i = 0; i < BLOCK_NUMS; ++ i) {
    free_block_tag[i] = false;
    bool is_free = true;
    for (int j = 0; j < PAGE_NUMS; ++ j)
      if (page_info_[i][j].status != 0)
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
  // destory page_info_
  for (int i = 0; i < BLOCK_NUMS; ++ i)
    delete[] page_info_[i];
  delete[] page_info_;

  // destory page_table
  delete[] page_table;

  // destory next_block and prev_block
  delete[] next_block;
  delete[] prev_block;

  delete[] free_block_tag;
}

char* Flash::Read(const size_t lba) {
  // calculate block num and page num
  PBA pba = page_info_[lba];
  size_t block_num_ = pba.block_num_;
  size_t page_num_ = pba.page_num_;
  // read page
  size_t lba_;
  char *data = ReadByPageNum(block_num, page_num, lba_, data);
  return data;
}

void Flash::Write(const size_t lba, const char* data) {
  // calculate block num and page num
  size_t block_num = lba / PAGE_NUMS;
  size_t page_num = lba % PAGE_NUMS;
  // examinate block whether used or not
  // if it is replace block, find corresponding primary block
  if () { 

  }
  // write page
  WriteByPageNum(block_num, page_num, lba, data);
  // update block info
  page_info_[block_num][page_num].status = 1;
  page_info_[block_num][page_num].lba = lba;
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

char* Flash::ReadByPageNum(const size_t block_num, const size_t page_num, size_t& lba) {
  char* data = new char[PAGE_SIZE];
  std::fstream f(GetBlockPath(block_num), std::ios::in);
  f.seekp(page_num * (PAGE_SIZE * sizeof(char) + sizeof(size_t)));
  f.read(data, PAGE_SIZE * sizeof(char));
  f.read((char *)&lba, sizeof(size_t));
  f.close();
  data[PAGE_SIZE] = '\0';
  // write log
  char log[LOG_LENGTH];
  sprintf(log, "READ\t%d\t%d\t%d\t%s\n", lba, block_num, page_num, data);
  WriteLog(log);
  return data;
}

void Flash::WriteByPageNum(const size_t block_num, const size_t page_num, const size_t lba, const char* data) {
  char* ndata = new char[PAGE_SIZE];
  strcpy(ndata, data);
  std::fstream f(GetBlockPath(block_num), std::ios::out | std::ios::in);
  f.seekp(page_num * (PAGE_SIZE * sizeof(char) + sizeof(size_t)));
  f.write(ndata, PAGE_SIZE * sizeof(char));
  f.write((char *)&lba, sizeof(size_t));
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
    page_info_[block_num][j] = BlockInfo();
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
      if (page_info_[i][j].status == 1) {
        while (page_index < PAGE_NUMS && page_info_[now_block][page_index].status != 0)
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
        
        page_info_[now_block][page_index].status = 1;
        page_info_[now_block][page_index].lba = lba;
        if (free_block_tag[now_block]) {
          free_block_tag[now_block] = false;
          free_blocks_num = free_blocks_num - 1;
        }
        // update page table
        page_table[lba].SetData(now_block, page_index);
      }
      // update block status
      page_info_[i][j] = BlockInfo();
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

std::pair<size_t, size_t> Flash::FindReplacement(const size_t block_num, const size_t page_num) {

}

}