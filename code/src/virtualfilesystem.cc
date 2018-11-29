#include "virtualfilesystem.h"

namespace bilsmtree {

FileSystem::FileSystem(FlashResult *flashresult) {
  std::cout << "Constructor in Virtual FileSystem" << std::endl;
}

FileSystem::~FileSystem() {
}

void FileSystem::Create(const std::string filename) {
  if (Config::FILESYSTEM_LOG)
    std::cout << "Create File In FileSystem:" << filename << std::endl;
  std::fstream f(filename, std::ios::out | std::ios::trunc);
  f.close();
}

void FileSystem::Open(const std::string filename, const size_t mode) {
  if (Config::FILESYSTEM_LOG)
    std::cout << "Open File In FileSystem:" << filename << std::endl;
  if (file_buffer_.size() >= Config::FileSystemConfig::MAX_FILE_OPEN) {
    std::cout << "FILE OPEN MAX" << std::endl;
    assert(false);
  }

  int index = SearchInBuffer(filename);
  if (index != -1) {
    std::cout << "FILE \'" << filename << "\' IS OPEN" << std::endl;
    assert(false); 
  }

  FileStatus fs = FileStatus(filename, mode);
  file_buffer_.push_back(fs);

  if (fs.mode_ & Config::FileSystemConfig::APPEND_OPTION)
    Seek(filename, fs.filesize_);
}

void FileSystem::Seek(const std::string filename, const size_t offset) {
  int index = SearchInBuffer(filename);
  if (index == -1) {
    std::cout << "FILE " << filename << " IS NOT OPEN" << std::endl;
    assert(false);
  }
  FileStatus& fs = file_buffer_[index];
  fs.offset_ = offset;
}

std::string FileSystem::Read(const std::string filename, const size_t read_size) {
  // if (Config::FILESYSTEM_LOG) {
  //   std::cout << "Read File In FileSystem:" << filename << std::endl;
  //   std::cout << "Read Size:" << read_size << std::endl;
  // }
  int index = SearchInBuffer(filename);
  if (index == -1) {
    std::cout << "FILE IS NOT OPEN" << std::endl;
    assert(false);
  }

  FileStatus& fs = file_buffer_[index];
  if (!(fs.mode_ & Config::FileSystemConfig::READ_OPTION)) {
    std::cout << "CANNOT READ WITHOUT READ PERMISSION" << std::endl;
    assert(false);
  }
  char *temp = new char[read_size + 1];

  std::fstream f(filename, std::ios::binary | std::ios::in);
  f.seekg(fs.offset_);
  f.read(temp, read_size * sizeof(char));
  temp[read_size] = '\0';
  f.close();
  fs.offset_ = fs.offset_ + read_size;
  std::string data = std::string(temp, read_size);
  delete[] temp;
  return data;
}

void FileSystem::Write(const std::string filename, const char* data, const size_t write_size) {
  // if (Config::FILESYSTEM_LOG) {
  //   std::cout << "Write File In FileSystem:" << filename << std::endl;
  //   std::cout << "Write Size:" << write_size << std::endl;
  //   std::cout << "Write:" << data << std::endl;
  // }
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

  std::fstream f(filename, std::ios::binary | std::ios::out | std::ios::in);
  f.seekp(fs.offset_);
  f.write(data, write_size * sizeof(char));
  f.close();
  fs.offset_ = fs.offset_ + write_size;
  fs.filesize_ = fs.filesize_ + write_size;
}

void FileSystem::SetFileSize(const std::string filename, const size_t file_size) {
  int index = SearchInBuffer(filename);
  if (index == -1)
    return ;
  file_buffer_[index].filesize_ = file_size;
}

void FileSystem::Delete(const std::string filename) {
  if (Config::FILESYSTEM_LOG)
    std::cout << "Delete File In FileSystem:" << filename << std::endl;
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

}