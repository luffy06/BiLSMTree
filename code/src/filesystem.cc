#include "filesystem.h"

namespace bilsmtree {

FileSystem::FileSystem() {
  fat_ = new uint[Config::FileSystemConfig::MAX_BLOCK_NUMS];
  for (int i = 0; i < Config::FileSystemConfig::MAX_BLOCK_NUMS; ++ i)
    fat_[i] = i;

  for (int i = 0; i < Config::FileSystemConfig::MAX_BLOCK_NUMS; ++ i) 
    free_blocks_.push(i);
  
  flash_ = new Flash();
  open_number_ = 0;
  total_file_number_ = 0;
}

FileSystem::~FileSystem() {
  delete[] fat_;
  delete flash_;
}

uint FileSystem::Open(const std::string filename, const uint mode) {
  if (open_number_ >= Config::FileSystemConfig::MAX_FILE_OPEN) {
    std::cout << "FILE OPEN MAX" << std::endl;
    exit(-1);
  }

  uint file_number_ = 0;
  bool found = false;
  for (std::map<uint, FCB>::iterator it = fcbs_.begin(); it != fcbs_.end(); ++ it) {
    if (filename == it -> second.filename_) {
      file_number_ = it -> first;
      found = true;
    }
  }

  if (!found) {
    file_number_ = total_file_number_ ++;
    fcbs_[file_number_] = FCB(filename, AssignFreeBlocks(), static_cast<uint>(0));
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

void FileSystem::Seek(const uint file_number, const uint offset) {
  uint lba_ = fcbs_[file_number].block_start_;
  uint offset_ = offset;
  while (offset_ >= Config::FileSystemConfig::BLOCK_SIZE && fat_[lba_] != lba_) {
    offset_ = offset_ - Config::FileSystemConfig::BLOCK_SIZE;
    lba_ = fat_[lba_];
  }
  int index = BinarySearchInBuffer(file_number);
  if (index == -1) {
    std::cout << "FILE IS NOT OPEN" << std::endl;
    exit(-1);
  }
  FileStatus& fs = file_buffer_[index];
  fs.lba_ = lba_;
  fs.offset_ = offset_;
}

uint FileSystem::GetFileSize(const uint file_number) {
  return fcbs_[file_number].filesize_;
}

std::string FileSystem::Read(const uint file_number, const uint read_size) {
  int index = BinarySearchInBuffer(file_number);
  uint read_size_ = read_size;
  if (index == -1) {
    std::cout << "FILE IS NOT OPEN" << std::endl;
    exit(-1);
  }

  FileStatus& fs = file_buffer_[index];
  if (!(fs.mode_ & Config::FileSystemConfig::READ_OPTION)) {
    std::cout << "CANNOT READ WITHOUT READ PERMISSION" << std::endl;
    exit(-1);
  }
  
  std::string data = "";
  if (read_size_ <= Config::FileSystemConfig::BLOCK_SIZE - fs.offset_) {
    char *c_data = flash_ -> Read(fs.lba_);
    c_data[fs.offset_ + read_size_] = '\0';
    data = data + std::string(c_data + fs.offset_);
    fs.offset_ = fs.offset_ + read_size_;
  }
  else {
    // read left offset
    char *c_data = flash_ -> Read(fs.lba_);
    c_data[Config::FileSystemConfig::BLOCK_SIZE] = '\0';
    data = data + std::string(c_data + fs.offset_);
    read_size_ = read_size_ - (Config::FileSystemConfig::BLOCK_SIZE - fs.offset_);
    fs.lba_ = fat_[fs.lba_];
    fs.offset_ = 0;
    // read total block
    while (read_size_ >= Config::FileSystemConfig::BLOCK_SIZE && fat_[fs.lba_] != fs.lba_) {
      c_data = flash_ -> Read(fs.lba_);
      c_data[Config::FileSystemConfig::BLOCK_SIZE] = '\0';
      data = data + std::string(c_data);
      read_size_ = read_size_ - Config::FileSystemConfig::BLOCK_SIZE;
      fs.lba_ = fat_[fs.lba_];
      fs.offset_ = 0;
    }
    // read left read_size
    if (read_size_ > 0) {
      c_data = flash_ -> Read(fs.lba_);
      c_data[read_size_] = '\0';
      data = data + std::string(c_data);
      fs.offset_ = read_size_;
    }
  }
  return data;
}

void FileSystem::Write(const uint file_number, const std::string data, const uint write_size) {
  if (data.size() == 0)
    return ;

  int index = BinarySearchInBuffer(file_number);
  if (index == -1) {
    std::cout << "FILE IS NOT OPEN" << std::endl;
    exit(-1);
  }

  FileStatus& fs = file_buffer_[index];
  if (!(fs.mode_ & (Config::FileSystemConfig::WRITE_OPTION | Config::FileSystemConfig::APPEND_OPTION))) {
    std::cout << "CANNOT WRITE WITHOUT WRITE PERMISSION" << std::endl;
    exit(-1);
  }

  uint need_blocks = (write_size % Config::FileSystemConfig::BLOCK_SIZE == 0 ? 
    write_size / Config::FileSystemConfig::BLOCK_SIZE : write_size / Config::FileSystemConfig::BLOCK_SIZE + 1);
  if (fs.mode_ & Config::FileSystemConfig::APPEND_OPTION)
    Seek(file_number, fcbs_[file_number].filesize_);

  uint size_ = 0;
  if (fs.offset_ < Config::FileSystemConfig::BLOCK_SIZE) {
    // write [0, write_size) or [0, BLOCK_SIZE - fs.offset_)
    char *c_data = flash_ -> Read(fs.lba_);
    std::string w_data = std::string(c_data) + 
                        data.substr(0, std::min(write_size, Config::FileSystemConfig::BLOCK_SIZE - fs.offset_));
    flash_ -> Write(fs.lba_, w_data.c_str());
    size_ = size_ + std::min(write_size, Config::FileSystemConfig::BLOCK_SIZE - fs.offset_);
    uint new_lba_ = AssignFreeBlocks();
    fat_[fs.lba_] = new_lba_;
    fs.lba_ = new_lba_;
    fs.offset_ = 0;
  }  
  while (size_ + Config::FileSystemConfig::BLOCK_SIZE <= write_size) {
    // write [size_, size_ + BLOCK_SIZE)
    flash_ -> Write(fs.lba_, data.substr(size_, size_ + Config::FileSystemConfig::BLOCK_SIZE).c_str());
    size_ = size_ + Config::FileSystemConfig::BLOCK_SIZE;
    uint new_lba_ = AssignFreeBlocks();
    fat_[fs.lba_] = new_lba_;
    fs.lba_ = new_lba_;
    fs.offset_ = 0;
  }
  if (size_ < write_size) {
    flash_ -> Write(fs.lba_, data.substr(size_, write_size).c_str());
    fs.offset_ = write_size - size_;
  }
  fcbs_[file_number].filesize_ = fcbs_[file_number].filesize_ + write_size;
}

void FileSystem::Delete(const std::string filename) {
  uint file_number_ = 0;
  bool found = false;
  for (std::map<uint, FCB>::iterator it = fcbs_.begin(); it != fcbs_.end(); ++ it) {
    if (filename == it -> second.filename_) {
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
    uint lba_ = fcbs_[file_number_].block_start_;
    while (fat_[lba_] != lba_) {
      free_blocks_.push(lba_);
      uint old_lba_ = lba_;
      lba_ = fat_[lba_];
      fat_[old_lba_] = old_lba_;
    }
    free_blocks_.push(lba_);
    fcbs_.erase(file_number_);
  }
}

void FileSystem::Close(const uint file_number) {
  int index = BinarySearchInBuffer(file_number);
  if (index == -1)
    return ;

  open_number_ --;
  file_buffer_.erase(file_buffer_.begin() + index); // TODO: LOW EFFCIENCY
}

int FileSystem::BinarySearchInBuffer(const uint file_number) {
  if (file_buffer_.size() == 0)
    return -1;
  uint l = 0;
  uint r = file_buffer_.size() - 1;
  if (file_number < file_buffer_[l].file_number_)
    return -1;
  else if (file_number > file_buffer_[r].file_number_)
    return -1;
  else if (file_number == file_buffer_[r].file_number_)
    return r;
  while (r - l > 1) {
    uint m = (l + r) / 2;
    if (file_buffer_[m].file_number_ <= file_number)
      l = m;
    else
      r = m;
  }
  if (file_buffer_[l].file_number_ == file_number)
    return l;
  return -1;
}

uint FileSystem::AssignFreeBlocks() {
  if (free_blocks_.empty()) {
    std::cout << "NOT ENOUGH FREE BLOCKS" << std::endl;
    exit(-1);
  }
  uint new_block = free_blocks_.front();
  free_blocks_.pop();
  return new_block;
}

void FileSystem::FreeBlock(const uint lba) {
  fat_[lba] = lba;
  free_blocks_.push(lba);
}

}