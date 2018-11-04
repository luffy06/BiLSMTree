namespace bilsmtree {

FileSystem::FileSystem() {
  fat_ = new size_t[MAX_BLOCK_NUMS];
  for (int i = 0; i < MAX_BLOCK_NUMS; ++ i)
    fat_[i] = i;

  for (int i = 0; i < MAX_BLOCK_NUMS; ++ i) 
    free_blocks_.push(i);
  
  flash_ = new Flash_();
  open_number_ = 0;
}

FileSystem::~FileSystem() {
  delete[] fat_;
  delete flash_;
}

size_t FileSystem::Open(const std::string& filename, const size_t mode) {
  if (open_number_ >= MAX_FILE_OPEN) {
    std::cout << "FILE OPEN MAX" << std::endl;
    exit(-1);
  }

  size_t file_number_ = 0;
  bool found = false;
  for (size_t i = 0; i < fcbs_.size(); ++ i) {
    if (filename == fcbs_[i].filename) {
      file_number_ = i;
      found = true;
    }
  }

  if (!found) {
    FCB fcb = FCB(filename, AssignFreeBlocks(), 0);
    fcbs_.push_back(fcb);
    file_number_ = fcbs_.size() - 1;
  }

  int index = BinarySearchInBuffer(file_number_);
  if (index != -1) {
    std::cout << "FILE \'" << filename << "\' IS OPEN" << std::endl;
    exit(-1); 
  }

  FileStatus fs;
  fs.file_number_ = file_number_;
  fs.lba_ = fcbs_[file_number_].block_start;
  fs.mode_ = mode;

  file_buffer_.push_back(fs);
  open_number_ ++;
  std::sort(file_buffer_.begin(), file_buffer_.end(), [](const FileStatus& fs1, const FileStatus& fs2) -> bool { 
    return fs1.file_number_ < fs2.file_number_;
  });
  return file_number_;
}

void FileSystem::Seek(const size_t file_number, const size_t offset) {
  
}

std::string FileSystem::Read(const size_t file_number, const size_t read_size) {
  int size = read_size;
  int index = BinarySearchInBuffer(file_number);
  if (index == -1) {
    std::cout << "FILE IS NOT OPEN" << std::endl;
    exit(-1);
  }

  FileStatus& fs = file_buffer_[index];
  if (!(fs.mode & Config::READ_OPTION)) {
    std::cout << "CANNOT READ WITHOUT READ PERMISSION" << std::endl;
    exit(-1);
  }

  std::string data = "";
  while (size > 0 && fat_[fs.lba_] != fs.lba_) {
    char* c_data = flash_->Read(fs.lba_);
    c_data[std::min(size, static_cast<int>(BLOCK_SIZE))] = '\0';
    data = data + std::string(c_data);
    size = size - BLOCK_SIZE;
    fs.lba_ = fat_[fs.lba_];
  }
  return data;
}

void FileSystem::Write(const size_t file_number, const std::string& data, const size_t write_size) {
  if (data.size() == 0)
    return ;

  int index = BinarySearchInBuffer(file_number);
  if (index == -1) {
    std::cout << "FILE IS NOT OPEN" << std::endl;
    exit(-1);
  }

  FileStatus& fs = file_buffer_[index];
  if (!(fs.mode & (Config::WRITE_OPTION | Config::APPEND_OPTION))) {
    std::cout << "CANNOT WRITE WITHOUT WRITE PERMISSION" << std::endl;
    exit(-1);
  }

  size_t need_blocks = (data.size() % BLOCK_SIZE == 0 ? data.size() / BLOCK_SIZE : data.size() / BLOCK_SIZE + 1);
  // std::cout << "NEED BLOCKS:" << need_blocks << std::endl;
  if (fs.mode & Config::APPEND_OPTION) {
    while (fs.lba_ != fat_[fs.lba_]) fs.lba_ = fat_[fs.lba_];

    for (size_t i = 0; i < need_blocks; ++ i) {
      size_t l = i * BLOCK_SIZE;
      size_t r = std::min(data.size(), (i + 1) * BLOCK_SIZE);
      flash_->Write(fs.lba_, data.substr(l, r).c_str()); // TODO: LOW EFFCIENCY
      size_t new_block = AssignFreeBlocks();
      fat_[fs.lba_] = new_block;
      fs.lba_ = fat_[fs.lba_];
    }
  }
  else {
    fs.lba_ = fcbs_[fs.file_number_].block_start_;
    for (size_t i = 0, j = 0; j < need_blocks && fs.lba_ != fat_[fs.lba_]; ++ i, ++ j) {
      size_t l = i * BLOCK_SIZE;
      size_t r = std::min(data.size(), (i + 1) * BLOCK_SIZE);
      flash_->Write(fs.lba_, data.substr(l, r).c_str()); // TODO: LOW EFFCIENCY
      fs.lba_ = fat_[fs.lba_];
    }
    if (j < need_blocks) {
      for (int i = 0; i < need_blocks; ++ i) {
        int l = i * BLOCK_SIZE;
        int r = std::min((int)data.size(), (i + 1) * BLOCK_SIZE);
        flash_->Write(fs.lba_, data.substr(l, r).c_str()); // TODO: LOW EFFCIENCY
        int new_block = AssignFreeBlocks();
        fat_[fs.lba_] = new_block;
        fs.lba_ = fat_[fs.lba_];
      }
    }
    else if (fs.lba_ != fat_[fs.lba_]) {
      int lba_ = fs.lba_;
      while (lba_ != fat_[lba_]) {
        int next_lba_ = fat_[lba_];
        FreeBlock(lba_);
        lba_ = next_lba;
      }
    }
  }
}

void FileSystem::Close(const size_t file_number) {
  int index = BinarySearchInBuffer(file_number);
  if (index == -1)
    return ;

  open_number_ --;
  file_buffer_.erase(file_buffer_.begin() + index); // TODO: LOW EFFCIENCY
}

int FileSystem::BinarySearchInBuffer(const size_t file_number) {
  if (file_buffer_.size() == 0)
    return -1;
  int l = 0;
  int r = file_buffer_.size() - 1;
  if (file_number < file_buffer_[l].file_number_)
    return -1;
  else if (file_number > file_buffer_[r].file_number_)
    return -1;
  else if (file_number == file_buffer_[r].file_number_)
    return r;
  while (r - l > 1) {
    int m = (l + r) / 2;
    if (file_buffer_[m].file_number_ <= file_number)
      l = m;
    else
      r = m;
  }
  if (file_buffer_[l].file_number_ == file_number)
    return l;
  return -1;
}

size_t FileSystem::AssignFreeBlocks() {
  if (free_blocks_.empty()) {
    std::cout << "NOT ENOUGH FREE BLOCKS" << std::endl;
    exit(-1);
  }
  size_t new_block = free_blocks_.front();
  free_blocks_.pop();
  return new_block;
}

void FileSystem::FreeBlock(const size_t lba) {
  fat_[lba] = lba;
  free_blocks_.push(lba);
}

}