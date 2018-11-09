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

class Flash;

struct FCB {
  std::string filename_;
  size_t block_start_;      // the start of block
  size_t filesize_;         // the number of blocks

  FCB() {
    std::cout << "EMPTY FCB" << std::endl;
  }

  FCB(const std::string& a, size_t b, size_t c) {
    filename_ = a;
    block_start_ = b;
    filesize_ = c;
  }
};

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
  FileSystem();
  ~FileSystem();

  static size_t Open(const std::string filename, const size_t mode);

  static void Seek(const size_t file_number, const size_t offset);

  static size_t GetFileSize(const size_t file_number);

  static std::string Read(const size_t file_number, const size_t read_size);

  static void Write(const size_t file_number, const std::string data, const size_t write_size);

  static void Delete(const std::string filename);

  static void Close(const size_t file_number);  
private:
  static std::vector<FileStatus> file_buffer_;
  static std::map<size_t, FCB> fcbs_;
  static size_t *fat_;
  static size_t total_file_number_;
  static size_t open_number_;
  static Flash *flash_;
  static std::queue<size_t> free_blocks_;

  static size_t AssignFreeBlocks();

  static void FreeBlock(const size_t lba);

  static int BinarySearchInBuffer(const size_t file_number);
};
}

#endif