#ifndef BILSMTREE_FILESYSTEM_H
#define BILSMTREE_FILESYSTEM_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <algorithm>

#include "flash.h"

namespace bilsmtree {

// file meta
struct FCB {
  std::string filename_;
  size_t block_start_;      // the start of block
  size_t filesize_;         // the number of blocks

  FCB() {
  }

  FCB(const std::string a, size_t b, size_t c) {
    filename_ = a;
    block_start_ = b;
    filesize_ = c;
  }
};

// opened file meta
struct FileStatus {
  size_t file_number_;
  size_t lba_;              // current block number
  size_t offset_;           // current block offset
  size_t mode_;
  
  FileStatus() {
    file_number_ = 0;
    lba_ = 0;
    offset_ = 0;
    mode_ = 0;
  }
};

class FileSystem {
public:
  FileSystem(FlashResult *flashresult);
  
  ~FileSystem();

  size_t Open(const std::string filename, const size_t mode);

  void Seek(const size_t file_number, const size_t offset);

  size_t GetFileSize(const size_t file_number);

  std::string Read(const size_t file_number, const size_t read_size);

  void Write(const size_t file_number, const char* data, const size_t write_size);

  void Delete(const std::string filename);

  void Close(const size_t file_number);  
private:
  std::vector<FileStatus> file_buffer_;     // opened files
  std::map<size_t, FCB> fcbs_;              // all files
  size_t *fat_;
  size_t total_file_number_;
  size_t open_number_;
  Flash *flash_;
  std::queue<size_t> free_blocks_;

  size_t AssignFreeBlocks();

  void FreeBlock(const size_t lba);

  int BinarySearchInBuffer(const size_t file_number);
};
}

#endif