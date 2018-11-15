#include "filesystem.h"

namespace bilsmtree {

FileSystem::FileSystem(FlashResult *flashresult) {
  fat_ = new size_t[Config::FileSystemConfig::MAX_BLOCK_NUMS];
  for (size_t i = 0; i < Config::FileSystemConfig::MAX_BLOCK_NUMS; ++ i)
    fat_[i] = i;

  for (size_t i = 0; i < Config::FileSystemConfig::MAX_BLOCK_NUMS; ++ i) 
    free_blocks_.push(i);
  
  flash_ = new Flash(flashresult);
  open_number_ = 0;
  total_file_number_ = 0;
}

FileSystem::~FileSystem() {
  delete[] fat_;
  delete flash_;
}

size_t FileSystem::Open(const std::string filename, const size_t mode) {
  std::cout << "OPEN " << filename << std::endl;
  if (open_number_ >= Config::FileSystemConfig::MAX_FILE_OPEN) {
    std::cout << "FILE OPEN MAX" << std::endl;
    exit(-1);
  }

  size_t file_number_ = 0;
  bool found = false;
  for (std::map<size_t, FCB>::iterator it = fcbs_.begin(); it != fcbs_.end(); ++ it) {
    if (filename == it->second.filename_) {
      file_number_ = it->first;
      found = true;
    }
  }

  if (!found) {
    file_number_ = total_file_number_ ++;
    fcbs_[file_number_] = FCB(filename, AssignFreeBlocks(), static_cast<size_t>(0));
    std::cout << "START LBA:" << fcbs_[file_number_].block_start_ << std::endl;
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
  std::sort(file_buffer_.begin(), file_buffer_.end(), [](const FileStatus& fs1, const FileStatus& fs2)->bool { 
    return fs1.file_number_ < fs2.file_number_;
  });
  return file_number_;
}

void FileSystem::Seek(const size_t file_number, const size_t offset) {
  std::cout << "Seek" << std::endl;
  size_t lba_ = fcbs_[file_number].block_start_;
  size_t offset_ = offset;
  while (offset_ >= Config::FileSystemConfig::BLOCK_SIZE) {
    offset_ = offset_ - Config::FileSystemConfig::BLOCK_SIZE;
    if (fat_[lba_] == lba_) {
      size_t new_lba_ = AssignFreeBlocks();
      fat_[lba_] = new_lba_;
    }
    lba_ = fat_[lba_];
  }

  assert(offset_ < Config::FileSystemConfig::BLOCK_SIZE);
  int index = BinarySearchInBuffer(file_number);
  if (index == -1) {
    std::cout << "FILE IS NOT OPEN" << std::endl;
    exit(-1);
  }
  FileStatus& fs = file_buffer_[index];
  fs.lba_ = lba_;
  fs.offset_ = offset_;
  std::cout << "FILE offset_:" << offset_ << std::endl;
}

size_t FileSystem::GetFileSize(const size_t file_number) {
  return fcbs_[file_number].filesize_;
}

std::string FileSystem::Read(const size_t file_number, const size_t read_size) {
  int index = BinarySearchInBuffer(file_number);
  size_t read_size_ = read_size;
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
    char *c_data = flash_->Read(fs.lba_);
    c_data[fs.offset_ + read_size_] = '\0';
    data = data + std::string(c_data + fs.offset_, read_size_);
    fs.offset_ = fs.offset_ + read_size_;
  }
  else {
    // read left offset
    char *c_data = flash_->Read(fs.lba_);
    c_data[Config::FileSystemConfig::BLOCK_SIZE] = '\0';
    data = data + std::string(c_data + fs.offset_, Config::FileSystemConfig::BLOCK_SIZE - fs.offset_);
    read_size_ = read_size_ - (Config::FileSystemConfig::BLOCK_SIZE - fs.offset_);
    fs.lba_ = fat_[fs.lba_];
    fs.offset_ = 0;
    // read total block
    while (read_size_ >= Config::FileSystemConfig::BLOCK_SIZE && fat_[fs.lba_] != fs.lba_) {
      c_data = flash_->Read(fs.lba_);
      c_data[Config::FileSystemConfig::BLOCK_SIZE] = '\0';
      data = data + std::string(c_data, Config::FileSystemConfig::BLOCK_SIZE);
      read_size_ = read_size_ - Config::FileSystemConfig::BLOCK_SIZE;
      fs.lba_ = fat_[fs.lba_];
      fs.offset_ = 0;
    }
    // read left read_size
    if (read_size_ > 0) {
      c_data = flash_->Read(fs.lba_);
      c_data[read_size_] = '\0';
      data = data + std::string(c_data, read_size_);
      fs.offset_ = read_size_;
    }
  }
  return data;
}

void FileSystem::Write(const size_t file_number, const char* data, const size_t write_size) {
  std::string data_(data, write_size);
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

  if (fs.mode_ & Config::FileSystemConfig::APPEND_OPTION)
    Seek(file_number, fcbs_[file_number].filesize_);

  size_t size_ = 0;
  if (fs.offset_ < Config::FileSystemConfig::BLOCK_SIZE) {
    // write [0, write_size) or [0, BLOCK_SIZE - fs.offset_)
    size_t add_ = std::min(write_size, Config::FileSystemConfig::BLOCK_SIZE - fs.offset_);
    char *c_data = flash_->Read(fs.lba_);
    std::string w_data = std::string(c_data, fs.offset_) + 
                        data_.substr(0, add_);
    flash_->Write(fs.lba_, w_data.c_str());
    size_ = size_ + add_;
    fs.offset_ = fs.offset_ + add_;
    if (fs.offset_ >= Config::FileSystemConfig::BLOCK_SIZE) {
      size_t new_lba_ = AssignFreeBlocks();
      fat_[fs.lba_] = new_lba_;
      fs.lba_ = new_lba_;
      fs.offset_ = 0;
    }
    
  }  
  while (size_ + Config::FileSystemConfig::BLOCK_SIZE < write_size) {
    // write [size_, size_ + BLOCK_SIZE)
    std::cout << data_.substr(size_, size_ + Config::FileSystemConfig::BLOCK_SIZE) << std::endl;
    flash_->Write(fs.lba_, data_.substr(size_, size_ + Config::FileSystemConfig::BLOCK_SIZE).c_str());
    size_ = size_ + Config::FileSystemConfig::BLOCK_SIZE;
    size_t new_lba_ = AssignFreeBlocks();
    fat_[fs.lba_] = new_lba_;
    fs.lba_ = new_lba_;
    fs.offset_ = 0;
  }
  if (size_ < write_size) {
    flash_->Write(fs.lba_, data_.substr(size_, write_size).c_str());
    fs.offset_ = write_size - size_;
  }
  fcbs_[file_number].filesize_ = fcbs_[file_number].filesize_ + write_size;
}

void FileSystem::Delete(const std::string filename) {
  size_t file_number_ = 0;
  bool found = false;
  for (std::map<size_t, FCB>::iterator it = fcbs_.begin(); it != fcbs_.end(); ++ it) {
    if (filename == it->second.filename_) {
      file_number_ = it->first;
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
    size_t lba_ = fcbs_[file_number_].block_start_;
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
  size_t l = 0;
  size_t r = file_buffer_.size() - 1;
  if (file_number < file_buffer_[l].file_number_)
    return -1;
  else if (file_number > file_buffer_[r].file_number_)
    return -1;
  else if (file_number == file_buffer_[r].file_number_)
    return r;
  while (r - l > 1) {
    size_t m = (l + r) / 2;
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
  assert(!free_blocks_.empty());
  size_t new_block = free_blocks_.front();
  free_blocks_.pop();
  return new_block;
}

void FileSystem::FreeBlock(const size_t lba) {
  fat_[lba] = lba;
  free_blocks_.push(lba);
}

}