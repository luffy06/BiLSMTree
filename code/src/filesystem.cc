#include "filesystem.h"

namespace bilsmtree {

FileSystem::FileSystem() {
  fat_ = new size_t[MAX_BLOCK_NUMS];
  for (int i = 0; i < MAX_BLOCK_NUMS; ++ i)
    fat_[i] = i;

  for (int i = 0; i < MAX_BLOCK_NUMS; ++ i) 
    free_blocks_.push(i);
  
  flash_ = new Flash_();
  open_number_ = 0;
  total_file_number_ = 0;
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
  for (std::map<size_t, FCB>::iterator it = fcbs_.begin(); it != fcbs_.end(); ++ it) {
    if (filename == it -> second.filename) {
      file_number_ = it -> first;
      found = true;
    }
  }

  if (!found) {
    FCB fcb = FCB(filename, AssignFreeBlocks(), 0);
    file_number_ = total_file_number_ ++;
    fcbs_[file_number_] = fcb;
  }

  int index = BinarySearchInBuffer(file_number_);
  if (index != -1) {
    std::cout << "FILE \'" << filename << "\' IS OPEN" << std::endl;
    exit(-1); 
  }

  FileStatus fs;
  fs.file_number_ = file_number_;
  fs.lba_ = fcbs_[file_number_].block_start_;
  fs.offset_ = 0;
  fs.mode_ = mode;

  file_buffer_.push_back(fs);
  open_number_ ++;
  std::sort(file_buffer_.begin(), file_buffer_.end(), [](const FileStatus& fs1, const FileStatus& fs2) -> bool { 
    return fs1.file_number_ < fs2.file_number_;
  });
  return file_number_;
}

void FileSystem::Seek(const size_t file_number, const size_t offset) {
  size_t lba_ = fcbs_[file_number].block_start_;
  while (offset >= BLOCK_SIZE && fat_[lba_] != lba_) {
    offset = offset - BLOCK_SIZE;
    lba_ = fat_[lba_];
  }
  int index = BinarySearchInBuffer(file_number);
  if (index == -1) {
    std::cout << "FILE IS NOT OPEN" << std::endl;
    exit(-1);
  }
  FileStatus& fs = file_buffer_[index];
  fs.lba_ = lba_;
  fs.offset_ = offset;
}

std::string FileSystem::Read(const size_t file_number, const size_t read_size) {
  int index = BinarySearchInBuffer(file_number);
  if (index == -1) {
    std::cout << "FILE IS NOT OPEN" << std::endl;
    exit(-1);
  }

  FileStatus& fs = file_buffer_[index];
  if (!(fs.mode & FileSystemConfig::READ_OPTION)) {
    std::cout << "CANNOT READ WITHOUT READ PERMISSION" << std::endl;
    exit(-1);
  }
  
  std::string data = "";
  if (read_size <= BLOCK_SIZE - fs.offset_) {
    char *c_data = flash_ -> Read(fs.lba_);
    c_data[fs.offset_ + read_size] = '\0';
    data = data + std::string(c_data + fs.offset_);
    fs.offset_ = fs.offset_ + read_size;
  }
  else {
    // read left offset
    char *c_data = flash_ -> Read(fs.lba_);
    c_data[BLOCK_SIZE] = '\0';
    data = data + std::string(c_data + fs.offset_);
    read_size = read_size - (BLOCK_SIZE - fs.offset_);
    fs.lba_ = fat_[fs.lba_];
    fs.offset_ = 0;
    // read total block
    while (read_size >= BLOCK_SIZE && fat_[fs.lba_] != fs.lba_) {
      c_data = flash_ -> Read(fs.lba_);
      c_data[BLOCK_SIZE] = '\0';
      data = data + std::string(c_data);
      size = size - BLOCK_SIZE;
      fs.lba_ = fat_[fs.lba_];
      fs.offset_ = 0;
    }
    // read left read_size
    if (read_size > 0) {
      c_data = flash_ -> Read(fs.lba_);
      c_data[read_size] = '\0';
      data = data + std::string(c_data);
      fs.offset_ = read_size;
    }
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

  size_t need_blocks = (write_size % BLOCK_SIZE == 0 ? write_size / BLOCK_SIZE : write_size / BLOCK_SIZE + 1);
  if (fs.mode & Config::APPEND_OPTION)
    Seek(file_number, fcbs_[file_number].filesize_);

  size_t size_ = 0;
  if (fs.offset_ < BLOCK_SIZE) {
    // write [0, write_size) or [0, BLOCK_SIZE - fs.offset_)
    char *c_data = flash_ -> Read(fs.lba_);
    c_data[BLOCK_SIZE] = '\0';
    std::string data = std::string(c_data) + data.substr(0, std::min(write_size, BLOCK_SIZE - fs.offset_));
    flash_ -> Write(fs.lba_, data.c_str());
    size_ = size_ + std::min(write_size, BLOCK_SIZE - fs.offset_);
    size_t new_lba_ = AssignFreeBlocks();
    fat_[fs.lba_] = new_lba_;
    fs.lba_ = new_lba_;
    fs.offset_ = 0;
  }  
  while (size_ + BLOCK_SIZE <= write_size) {
    // write [size_, size_ + BLOCK_SIZE)
    flash_ -> Write(fs.lba_, data.substr(size_, size_ + BLOCK_SIZE).c_str());
    size_ = size_ + BLOCK_SIZE;
    size_t new_lba_ = AssignFreeBlocks();
    fat_[fs.lba_] = new_lba_;
    fs.lba_ = new_lba_;
    fs.offset_ = 0;
  }
  if (size_ < write_size) {
    flash_ -> Write(fs.lba_, data.substr(size_, write_size),c_str());
    fs.offset_ = write_size - size_;
  }
  fcbs_[file_number].filesize_ = fcbs_[file_number].filesize_ + write_size;
}

void FileSystem::Delete(const std::string& filename) {
  size_t file_number_ = 0;
  bool found = false;
  for (std::map<size_t, FCB>::iterator it = fcbs_.begin(); it != fcbs_.end(); ++ it) {
    if (filename == it -> second.filename) {
      file_number_ = it -> first;
      found = true;
    }
  }

  if (found) {
    int index = BinarySearchInBuffer(file_number_);
    if (index == -1) {
      std::cout << "FILE IS NOT OPEN" << std::endl;
      exit(-1);
    }

    file_buffer_.erase(file_buffer_.begin() + index);
    size_t lba_ = fcbs_[file_number].block_start_;
    while (fat_[lba_] != lba_) {
      free_blocks_.push(lba_);
      size_t old_lba_ = lba_;
      lba_ = fat_[lba_];
      fat_[old_lba_] = old_lba_;
    }
    free_blocks_.push(lba_);
    fcbs_.erase(file_number_);
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