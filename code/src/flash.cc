#include "flash.h"

namespace bilsmtree {

Flash::Flash(FlashResult *flashresult) {
  // std::cout << "CREATING FLASH" << std::endl;
  // create block files
  for (size_t i = 0; i < Config::FlashConfig::BLOCK_NUMS; ++ i) {
    std::string block_path = GetBlockPath(i);
    std::fstream f(block_path, std::ios::app | std::ios::out);
    f.close();
  }

  std::fstream f(Config::TRACE_PATH, std::ios::ate | std::ios::out);
  f.close();  

  free_pages_num_ = Config::FlashConfig::BLOCK_NUMS * Config::FlashConfig::PAGE_NUMS;
  heap_ = 0;
  
  block_info_.resize(Config::FlashConfig::BLOCK_NUMS);
  for (size_t i = 0; i < Config::FlashConfig::BLOCK_NUMS; ++ i) {
    size_t id_ = i + 1;
    int father_ = id_ / 2 - 1;
    int left_ = (id_ * 2 < Config::FlashConfig::BLOCK_NUMS ? id_ * 2 : 0) - 1;
    int right_ = (id_ * 2 + 1 < Config::FlashConfig::BLOCK_NUMS ? id_ * 2 + 1 : 0) - 1;
    block_info_[i] = BlockInfo(i, father_, left_, right_);
  }
  page_info_.resize(Config::FlashConfig::BLOCK_NUMS);
  for (size_t i = 0; i < Config::FlashConfig::BLOCK_NUMS; ++ i)
    page_info_[i].resize(Config::FlashConfig::PAGE_NUMS);
  
  for (size_t i = 0; i < Config::FlashConfig::BLOCK_NUMS; ++ i) {
    for (size_t j = 0; j < Config::FlashConfig::PAGE_NUMS; ++ j)
      assert(page_info_[i][j].status_ == PageFree);
  }

  op_ = 0;
  op_numb_[0] = op_numb_[1] = 0;

  flashresult_ = flashresult;
}
  
Flash::~Flash() {
  std::stringstream ss;
  ss << op_numb_[0] << "\t" << op_numb_[1] << "\n";
  WriteLog(ss.str());
  if (buffer_.size() > 0) {
    std::fstream f(Config::TRACE_PATH, std::ios::app | std::ios::out);
    f << buffer_;
    f.close();
  }
}

char* Flash::Read(const size_t lba) {
  std::stringstream ss;
  ss << "R\t" << lba << "\n";
  WriteLog(ss.str());
  op_numb_[op_] = op_numb_[op_] + 1;
  // read block num and page num from page table
  if (page_table_.find(lba) == page_table_.end()) {
    std::cout << "LBA:" << lba << " doesn't exist!" << std::endl;
    assert(false);
  }
  PBA pba_ = page_table_[lba];
  // read page
  std::pair<size_t, char*> res = ReadByPageNum(pba_.block_num_, pba_.page_num_);
  assert(lba == res.first);
  return res.second;
}

void Flash::Write(const size_t lba, const char* data) {
  std::stringstream ss;
  ss << "W\t" << lba << "\n";
  WriteLog(ss.str());
  op_numb_[op_] = op_numb_[op_] + 1;
  // calculate block num and page num
  PBA pba_ = GetLocation();
  if (Config::FLASH_LOG)
    std::cout << "Write LBA:" << lba << "\tBLOCK NUM:" << pba_.block_num_ << "\tPAGE NUM:" << pba_.page_num_ << std::endl;
  // write page
  WriteByPageNum(pba_.block_num_, pba_.page_num_, lba, data);
  // check whether start garbage collection
  if (free_pages_num_ < Config::FlashConfig::BLOCK_COLLECTION_TRIGGER)
    MajorCollectGarbage();
}

void Flash::Invalidate(const size_t lba) {
  if (page_table_.find(lba) != page_table_.end()) {
    size_t old_block_num_ = page_table_[lba].block_num_;
    size_t old_page_num_ = page_table_[lba].page_num_;
    page_info_[old_block_num_][old_page_num_].status_ = PageInvalid;
    block_info_[old_block_num_].valid_nums_ = block_info_[old_block_num_].valid_nums_ - 1;
    block_info_[old_block_num_].invalid_nums_ = block_info_[old_block_num_].invalid_nums_ + 1;
    page_table_.erase(lba);
  }
}

std::pair<size_t, char*> Flash::ReadByPageNum(const size_t block_num, const size_t page_num) {
  size_t page_size_ = Config::FlashConfig::PAGE_SIZE;
  char *data_ = new char[page_size_ + 1];
  size_t lba_;
  std::fstream f(GetBlockPath(block_num), std::ios::in);
  f.seekp(page_num * (page_size_ * sizeof(char) + sizeof(size_t)));
  f.read(data_, page_size_ * sizeof(char));
  f.read((char *)&lba_, sizeof(size_t));
  f.close();
  data_[page_size_] = '\0';
  flashresult_->Read();
  return std::make_pair(lba_, data_);
}

void Flash::WriteByPageNum(const size_t block_num, const size_t page_num, const size_t lba, const char* data) {
  size_t page_size_ = Config::FlashConfig::PAGE_SIZE;
  char *temp = new char[page_size_ + 1];
  strncpy(temp, data, page_size_);
  std::fstream f(GetBlockPath(block_num), std::ios::out | std::ios::in);
  f.seekp(page_num * (page_size_ * sizeof(char) + sizeof(size_t)));
  f.write(temp, page_size_ * sizeof(char));
  f.write((char *)&lba, sizeof(size_t));
  f.close();
  delete[] temp;
  // make old location invalid
  Invalidate(lba);
  // update page info
  page_info_[block_num][page_num].status_ = PageValid;
  page_info_[block_num][page_num].lba_ = lba;
  block_info_[block_num].valid_nums_ = block_info_[block_num].valid_nums_ + 1;
  // update page table
  page_table_[lba] = PBA(block_num, page_num);
  // update free page number
  free_pages_num_ = free_pages_num_ - 1;
  AdjustHeap(block_num);
  flashresult_->Write();
}

void Flash::Erase(const size_t block_num) {
  if (Config::FLASH_LOG)
    std::cout << "ERASE " << block_num << std::endl;
  // erase block data in file
  std::fstream f(GetBlockPath(block_num), std::ios::ate | std::ios:: out);
  f.close();
  // erase page info
  for (size_t j = 0; j < Config::FlashConfig::PAGE_NUMS; ++ j)
    page_info_[block_num][j] = PageInfo();
  // erase block info
  block_info_[block_num].offset_ = 0;
  block_info_[block_num].invalid_nums_ = 0;
  block_info_[block_num].valid_nums_ = 0;
  // insert into free queue
  free_pages_num_ = free_pages_num_ + Config::FlashConfig::PAGE_NUMS;
  AdjustHeap(block_num);
  flashresult_->Erase();
}

void Flash::MajorCollectGarbage() {
  if (Config::FLASH_LOG) {
    std::cout << "MajorCollectGarbage In Flash" << std::endl;
    std::cout << "Before Free Page Number:" << free_pages_num_ << std::endl;
  }
  std::vector<BlockInfo> blocks_;
  size_t total_valids = 0;
  size_t total_invalids = 0;
  for (size_t i = 0; i < Config::FlashConfig::BLOCK_NUMS; ++ i) {
    total_valids = total_valids + block_info_[i].valid_nums_;
    total_invalids = total_invalids + block_info_[i].invalid_nums_;
    BlockInfo bi = block_info_[i];
    if (bi.invalid_nums_ != 0 || bi.valid_nums_ != 0) {
      if (bi.valid_nums_ == Config::FlashConfig::PAGE_NUMS)
        continue;
      blocks_.push_back(bi);
    }
  }
  sort(blocks_.begin(), blocks_.end(), [](const BlockInfo a, const BlockInfo b)->bool {
    if (a.invalid_nums_ != b.invalid_nums_)
      return a.invalid_nums_ > b.invalid_nums_;
    return a.valid_nums_ < b.valid_nums_;
  });

  std::vector<size_t> erase_list_;
  for (size_t i = 0; i < blocks_.size() && free_pages_num_ < Config::FlashConfig::BLOCK_COLLECTION_THRESOLD; ++ i) {
    size_t block_num_ = blocks_[i].block_num_;
    for (size_t j = 0; j < Config::FlashConfig::PAGE_NUMS; ++ j) {
      if (page_info_[block_num_][j].status_ != PageValid)
        continue;
      PBA pba_ = GetLocation();
      size_t lba_ = page_info_[block_num_][j].lba_;
      // read data
      std::pair<size_t, char*> res = ReadByPageNum(block_num_, j);
      assert(res.first == lba_);
      // write data
      WriteByPageNum(pba_.block_num_, pba_.page_num_, res.first, res.second);
    }
    Erase(block_num_);
  }
  if (Config::FLASH_LOG) {
    std::cout << "TOTAL VALID:" << total_valids << "\tINVALID:" << total_invalids << std::endl; 
    std::cout << "After Free Page Number:" << free_pages_num_ << std::endl;
    std::cout << "MajorCollectGarbage In Flash End" << std::endl;
  }
}

PBA Flash::GetLocation() {
  if (Config::FLASH_LOG)
    std::cout << "Total Free Page:" << free_pages_num_ << std::endl;
  for (size_t repeat_ = 0; repeat_ <= Config::FlashConfig::BLOCK_NUMS && block_info_[heap_].father_ != -1; ++ repeat_) {
    heap_ = block_info_[heap_].father_;
    assert(repeat_ < Config::FlashConfig::BLOCK_NUMS);
  }
  if (Config::FLASH_LOG)
    block_info_[heap_].Show();
  if (block_info_[heap_].offset_ >= Config::FlashConfig::PAGE_NUMS)
    ShowInfo();
  assert(heap_ >= 0 && heap_ < block_info_.size());
  assert(block_info_[heap_].offset_ >= 0 && block_info_[heap_].offset_ < Config::FlashConfig::PAGE_NUMS);
  PBA pba_ = PBA(heap_, block_info_[heap_].offset_);
  block_info_[heap_].offset_ = block_info_[heap_].offset_ + 1;
  return pba_;
}

void Flash::AdjustHeap(int block_num) {
  if (block_num == -1)
    return ;
  int father_ = block_info_[block_num].father_;
  int left_ = block_info_[block_num].left_;
  int right_ = block_info_[block_num].right_;
  int self_free_ = GetFreePageNumber(block_num);
  int father_free_ = GetFreePageNumber(father_);
  int left_free_ = GetFreePageNumber(left_);
  int right_free_ = GetFreePageNumber(right_);
  int ex_son_ = -1;
  int ot_son_ = -1;
  int son_free_ = -1;
  if (left_free_ > right_free_) {
    ex_son_ = left_;
    ot_son_ = right_;
    son_free_ = left_free_;
  }
  else {
    ex_son_ = right_;
    ot_son_ = left_;
    son_free_ = right_free_;
  }
  assert(self_free_ != -1);
  if (father_free_ != -1 &&  self_free_ > father_free_) {
    assert(father_ != -1);
    int other_son_ = (block_info_[father_].left_ == block_num ? block_info_[father_].right_ : block_info_[father_].left_);
    int f_father_ = block_info_[father_].father_;
    // update self
    block_info_[block_num].father_ = block_info_[father_].father_;
    block_info_[block_num].left_ = (block_num == block_info_[father_].left_ ? father_ : block_info_[father_].left_);
    block_info_[block_num].right_ = (block_num == block_info_[father_].right_ ? father_ : block_info_[father_].right_);
    // update father
    block_info_[father_].left_ = left_;
    block_info_[father_].right_ = right_;
    block_info_[father_].father_ = block_num;
    // update self son
    if (left_ != -1) block_info_[left_].father_ = father_;
    if (right_ != -1) block_info_[right_].father_ = father_;
    // update father's father
    if (f_father_ != -1) {
      if (block_info_[f_father_].left_ == father_)
        block_info_[f_father_].left_ = block_num;
      else
        block_info_[f_father_].right_ = block_num;
    }
    // update father's son
    if (other_son_ != -1) {
      block_info_[other_son_].father_ = block_num;
    }
    AdjustHeap(block_num);
  }
  else if (son_free_ != -1 && self_free_ < son_free_) {
    assert(ex_son_ != -1);
    int son_left_ = block_info_[ex_son_].left_;
    int son_right_ = block_info_[ex_son_].right_;
    // update self
    block_info_[block_num].father_ = ex_son_;
    block_info_[block_num].left_ = son_left_;
    block_info_[block_num].right_ = son_right_;
    // update exchanged son
    block_info_[ex_son_].father_ = father_;
    block_info_[ex_son_].left_ = (ex_son_ == left_ ? block_num : ot_son_);
    block_info_[ex_son_].right_ = (ex_son_ == right_ ? block_num : ot_son_);
    // update son's son
    if (son_left_ != -1) block_info_[son_left_].father_ = block_num;
    if (son_right_ != -1) block_info_[son_right_].father_ = block_num;
    // update father
    if (father_ != -1) {
      if (block_info_[father_].left_ == block_num)
        block_info_[father_].left_ = ex_son_;
      else
        block_info_[father_].right_ = ex_son_;
    }
    // update another son
    if (ot_son_ != -1) {
      block_info_[ot_son_].father_ = ex_son_;
    }
    AdjustHeap(block_num);
  }
}

int Flash::GetFreePageNumber(int block_num) {
  if (block_num == -1)
    return -1;
  return Config::FlashConfig::PAGE_NUMS - block_info_[block_num].valid_nums_ - block_info_[block_num].invalid_nums_;
}

void Flash::ShowInfo() {
  std::cout << "Show Info" << std::endl;
  for (size_t i = 0; i < Config::FlashConfig::BLOCK_NUMS; ++ i) {
    // std::cout << "block:" << block_info_[i].block_num_ << std::endl;
    // std::cout << block_info_[i].invalid_nums_ << "\t" << block_info_[i].valid_nums_ << std::endl;
    std::cout << "Self:" << i << "\tOffset:" << block_info_[i].offset_ << std::endl;
    std::cout << "Father:" << block_info_[i].father_ << "\tLeft:" << block_info_[i].left_ << "\tRight:" << block_info_[i].right_ << std::endl;
  }
}

void Flash::WriteLog(std::string data) {
  buffer_ = buffer_ + data;
  if (buffer_.size() >= Config::BUFFER_SIZE) {
    std::fstream f(Config::TRACE_PATH, std::ios::app | std::ios::out);
    f << buffer_;
    f.close();
    buffer_ = "";
  }
}

}