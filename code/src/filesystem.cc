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
  if (Config::FILESYSTEM_LOG)
    std::cout << "Open File:" << filename << std::endl;
  if (open_number_ >= Config::FileSystemConfig::MAX_FILE_OPEN) {
    std::cout << "FILE OPEN MAX" << std::endl;
    assert(false);
  }

  size_t file_number_ = 0;
  bool found = false;
  for (std::map<size_t, FCB>::iterator it = fcbs_.begin(); it != fcbs_.end(); ++ it) {
    if (filename == it->second.filename_) {
      file_number_ = it->first;
      found = true;
    }
  }

  if (!found) 
    std::cout << "File:" << filename << " doesn't exist" << std::endl;
  assert(found);

  int index = BinarySearchInBuffer(file_number_);
  if (index != -1) {
    std::cout << "FILE \'" << filename << "\' IS OPEN" << std::endl;
    assert(false); 
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

  if (fs.mode_ & Config::FileSystemConfig::APPEND_OPTION)
    Seek(file_number_, fcbs_[file_number_].filesize_);

  return file_number_;
}

void FileSystem::Create(const std::string filename) {
  if (Config::FILESYSTEM_LOG)
    std::cout << "Create File:" << filename << std::endl;
  bool found = false;
  for (std::map<size_t, FCB>::iterator it = fcbs_.begin(); it != fcbs_.end(); ++ it) {
    if (filename == it->second.filename_)
      found = true;
  }
  if (found) {
    std::cout << "File:" << filename << " exists" << std::endl;
  }
  assert(!found);
  size_t file_number_ = total_file_number_ ++;
  fcbs_[file_number_] = FCB(filename, AssignFreeBlocks(), static_cast<size_t>(0));
}

void FileSystem::Seek(const size_t file_number, const size_t offset) {
  if (Config::SEEK_LOG)
    std::cout << "Seek:" << fcbs_[file_number].filename_ << std::endl;
  size_t lba_ = fcbs_[file_number].block_start_;
  size_t offset_ = offset;
  while (offset_ >= Config::FileSystemConfig::BLOCK_SIZE) {
    offset_ = offset_ - Config::FileSystemConfig::BLOCK_SIZE;
    assert(fat_[lba_] != lba_);
    lba_ = fat_[lba_];
  }
  assert(offset_ < Config::FileSystemConfig::BLOCK_SIZE);
  int index = BinarySearchInBuffer(file_number);
  if (index == -1) {
    std::cout << "FILE " << fcbs_[file_number].filename_ << " IS NOT OPEN" << std::endl;
    assert(false);
  }
  FileStatus& fs = file_buffer_[index];
  fs.lba_ = lba_;
  fs.offset_ = offset_;
  // std::cout << "FILE offset_:" << offset_ << std::endl;
}

void FileSystem::SetFileSize(const size_t file_number, const size_t file_size) {
  fcbs_[file_number].filesize_ = file_size;
}

std::string FileSystem::Read(const size_t file_number, const size_t read_size) {
  // if (Config::FILESYSTEM_LOG)
  //   std::cout << "Read File:" << fcbs_[file_number].filename_ << std::endl;
  int index = BinarySearchInBuffer(file_number);
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
void FileSystem::Write(const size_t file_number, const char* data, const size_t write_size) {
  // if (Config::FILESYSTEM_LOG)
  //   std::cout << "Write File:" << fcbs_[file_number].filename_ << std::endl;
  std::string data_(data, write_size);
  int index = BinarySearchInBuffer(file_number);
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
  fcbs_[file_number].filesize_ = fcbs_[file_number].filesize_ + write_size;
}

void FileSystem::Delete(const std::string filename) {
  if (Config::FILESYSTEM_LOG)
    std::cout << "Delete File:" << filename << std::endl;
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
    if (index != -1) {
      file_buffer_.erase(file_buffer_.begin() + index);
    }

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
  if (free_blocks_.empty()) {
    for (std::map<size_t, FCB>::iterator it = fcbs_.begin(); it != fcbs_.end(); ++ it) {
      FCB fcb = (*it).second;
      std::cout << "File Name:" << fcb.filename_ << "\tFile Size:" << fcb.filesize_ << std::endl;
    }
  }
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