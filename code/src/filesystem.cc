#include "filesystem.h"

namespace bilsmtree {

FileSystem::FileSystem(FlashResult *flashresult) {
  fat_ = new size_t[Config::FileSystemConfig::MAX_BLOCK_NUMS];
  for (size_t i = 0; i < Config::FileSystemConfig::MAX_BLOCK_NUMS; ++ i)
    fat_[i] = i;

  for (size_t i = 0; i < Config::FileSystemConfig::MAX_BLOCK_NUMS; ++ i) 
    free_blocks_.push(i);
  
  flash_ = new Flash(flashresult);
  std::cout << "Constructor in FileSystem" << std::endl;
}

FileSystem::~FileSystem() {
  delete[] fat_;
  delete flash_;
}

void FileSystem::Create(const std::string filename) {
  if (Config::FILESYSTEM_LOG)
    std::cout << "Create File:" << filename << std::endl;

  if (fcbs_.find(filename) != fcbs_.end()) {
    std::cout << "File:" << filename << " exists" << std::endl;
    assert(false);
  }
  FCB fcb = FCB(AssignFreeBlocks(), static_cast<size_t>(0));
  fcbs_.insert(std::make_pair(filename, fcb));
}

void FileSystem::Open(const std::string filename, const size_t mode) {
  if (Config::FILESYSTEM_LOG)
    std::cout << "Open File:" << filename << std::endl;
  if (file_buffer_.size() >= Config::FileSystemConfig::MAX_FILE_OPEN) {
    std::cout << "FILE OPEN MAX" << std::endl;
    assert(false);
  }

  if (fcbs_.find(filename) == fcbs_.end()) {
    std::cout << "File:" << filename << " doesn't exist" << std::endl;
    assert(false);
  }

  int index = SearchInBuffer(filename);
  if (index != -1) {
    std::cout << "FILE \'" << filename << "\' IS OPEN" << std::endl;
    assert(false); 
  }

  FileStatus fs = FileStatus(filename);
  fs.lba_ = fcbs_[filename].block_start_;
  fs.offset_ = 0;
  fs.mode_ = mode;

  file_buffer_.push_back(fs);

  if (fs.mode_ & Config::FileSystemConfig::APPEND_OPTION)
    Seek(filename, fcbs_[filename].filesize_);
}

void FileSystem::Seek(const std::string filename, const size_t offset) {
  size_t lba_ = fcbs_[filename].block_start_;
  size_t offset_ = offset;
  while (offset_ >= Config::FileSystemConfig::BLOCK_SIZE) {
    offset_ = offset_ - Config::FileSystemConfig::BLOCK_SIZE;
    assert(fat_[lba_] != lba_);
    lba_ = fat_[lba_];
  }
  assert(offset_ < Config::FileSystemConfig::BLOCK_SIZE);
  int index = SearchInBuffer(filename);
  if (index == -1) {
    std::cout << "FILE " << filename << " IS NOT OPEN" << std::endl;
    assert(false);
  }
  FileStatus& fs = file_buffer_[index];
  fs.lba_ = lba_;
  fs.offset_ = offset_;
}

void FileSystem::SetFileSize(const std::string filename, const size_t file_size) {
  fcbs_[filename].filesize_ = file_size;
}

std::string FileSystem::Read(const std::string filename, const size_t read_size) {
  // if (Config::FILESYSTEM_LOG)
  //   std::cout << "Read File:" << filename << std::endl;
  int index = SearchInBuffer(filename);
  size_t read_size_ = read_size;
  if (index == -1) {
    std::cout << "FILE IS NOT OPEN" << std::endl;
    assert(false);
  }

  FileStatus& fs = file_buffer_[index];
  if (!(fs.mode_ & Config::FileSystemConfig::READ_OPTION)) {
    std::cout << "CANNOT READ WITHOUT READ PERMISSION" << std::endl;
    assert(false);
  }
  
  // std::cout << "Offset:" << fs.offset_ << "\t" << fcbs_[fs.file_number_].filesize_ << std::endl;
  std::string data = "";
  if (read_size_ <= Config::FileSystemConfig::BLOCK_SIZE - fs.offset_) {
    // std::cout << "Read 1" << std::endl;
    char *c_data = flash_->Read(fs.lba_);
    data = data + std::string(c_data + fs.offset_, read_size_);
    delete[] c_data;
    fs.offset_ = fs.offset_ + read_size_;
  }
  else {
    // read left offset
    // std::cout << "Read 2" << std::endl;
    char *c_data = flash_->Read(fs.lba_);
    c_data[Config::FileSystemConfig::BLOCK_SIZE] = '\0';
    data = data + std::string(c_data + fs.offset_, Config::FileSystemConfig::BLOCK_SIZE - fs.offset_);
    delete[] c_data;
    read_size_ = read_size_ - (Config::FileSystemConfig::BLOCK_SIZE - fs.offset_);
    fs.lba_ = fat_[fs.lba_];
    fs.offset_ = 0;
    // read total block
    while (read_size_ >= Config::FileSystemConfig::BLOCK_SIZE && fat_[fs.lba_] != fs.lba_) {
      // std::cout << "Read while" << std::endl;
      c_data = flash_->Read(fs.lba_);
      c_data[Config::FileSystemConfig::BLOCK_SIZE] = '\0';
      data = data + std::string(c_data, Config::FileSystemConfig::BLOCK_SIZE);
      delete[] c_data;
      read_size_ = read_size_ - Config::FileSystemConfig::BLOCK_SIZE;
      fs.lba_ = fat_[fs.lba_];
      fs.offset_ = 0;
    }
    // read left read_size
    if (read_size_ > 0) {
      // std::cout << "Read 3" << std::endl;
      c_data = flash_->Read(fs.lba_);
      c_data[read_size_] = '\0';
      data = data + std::string(c_data, read_size_);
      delete[] c_data;
      fs.offset_ = read_size_;
    }
  }
  // std::cout << "Data Read From FileSystem:" << data << std::endl;
  return data;
}

// TODO: Using substr() is LOW EFFICIENCY
void FileSystem::Write(const std::string filename, const char* data, const size_t write_size) {
  // if (Config::FILESYSTEM_LOG)
  //   std::cout << "Write File:" << filename << std::endl;
  std::string data_(data, write_size);
  int index = SearchInBuffer(filename);
  if (index == -1) {
    std::cout << "FILE IS NOT OPEN" << std::endl;
    assert(false);
  }

  FileStatus& fs = file_buffer_[index];
  if (!(fs.mode_ & (Config::FileSystemConfig::WRITE_OPTION | Config::FileSystemConfig::APPEND_OPTION))) {
    std::cout << "CANNOT WRITE WITHOUT WRITE PERMISSION" << std::endl;
    assert(false);
  }

  assert(fs.offset_ < Config::FileSystemConfig::BLOCK_SIZE);
  size_t size_ = 0;
  if (fs.offset_ > 0) {
    // write [0, write_size) or [0, BLOCK_SIZE - fs.offset_)
    size_t add_ = std::min(write_size, Config::FileSystemConfig::BLOCK_SIZE - fs.offset_);
    // std::cout << "Read From FileSystem Write" << std::endl;
    char *c_data = flash_->Read(fs.lba_);
    // std::cout << "Origin data:" << c_data << std::endl;
    // std::cout << fs.offset_ << "\t" << add_ << std::endl;
    // std::cout << "Concat:" << data_.substr(0, add_) << std::endl;
    std::string w_data = std::string(c_data, fs.offset_) + 
                        data_.substr(0, add_);
    flash_->Write(fs.lba_, w_data.c_str());
    size_ = size_ + add_;
    fs.offset_ = fs.offset_ + add_;
    if (fs.offset_ == Config::FileSystemConfig::BLOCK_SIZE) {
      if (fat_[fs.lba_] == fs.lba_) {
        size_t new_lba_ = AssignFreeBlocks();
        fat_[fs.lba_] = new_lba_;
      }
      fs.lba_ = fat_[fs.lba_];
      fs.offset_ = 0;
    }
    else {
      return ;
    }
  }
  assert(fs.offset_ == 0);
  while (size_ + Config::FileSystemConfig::BLOCK_SIZE <= write_size) {
    // write [size_, size_ + BLOCK_SIZE)
    // std::cout << data_.substr(size_, size_ + Config::FileSystemConfig::BLOCK_SIZE) << std::endl;
    flash_->Write(fs.lba_, data_.substr(size_, size_ + Config::FileSystemConfig::BLOCK_SIZE).c_str());
    size_ = size_ + Config::FileSystemConfig::BLOCK_SIZE;
    if (fat_[fs.lba_] == fs.lba_) {
      size_t new_lba_ = AssignFreeBlocks();
      fat_[fs.lba_] = new_lba_;
    }
    fs.lba_ = fat_[fs.lba_];
    fs.offset_ = 0;
  }
  if (size_ < write_size) {
    flash_->Write(fs.lba_, data_.substr(size_, write_size).c_str());
    fs.offset_ = write_size - size_;
  }
  fcbs_[filename].filesize_ = fcbs_[filename].filesize_ + write_size;
}

void FileSystem::Delete(const std::string filename) {
  if (Config::FILESYSTEM_LOG)
    std::cout << "Delete File:" << filename << std::endl;

  if (fcbs_.find(filename) != fcbs_.end()) {
    int index = SearchInBuffer(filename);
    if (index != -1) {
      file_buffer_.erase(file_buffer_.begin() + index);
    }

    size_t lba_ = fcbs_[filename].block_start_;
    while (fat_[lba_] != lba_) {
      free_blocks_.push(lba_);
      size_t old_lba_ = lba_;
      lba_ = fat_[lba_];
      fat_[old_lba_] = old_lba_;
    }
    free_blocks_.push(lba_);
    fcbs_.erase(filename);
  }
}

void FileSystem::Close(const std::string filename) {
  int index = SearchInBuffer(filename);
  if (index == -1)
    return ;
  file_buffer_.erase(file_buffer_.begin() + index); // TODO: LOW EFFCIENCY
}

int FileSystem::SearchInBuffer(const std::string filename) {
  for (size_t i = 0; i < file_buffer_.size(); ++ i) {
    if (file_buffer_[i].filename_ == filename)
      return i;
  }
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