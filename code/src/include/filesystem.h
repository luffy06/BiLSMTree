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
  size_t block_start_;      // the start of block
  size_t filesize_;         // the number of blocks

  FCB() {
  }

  FCB(size_t b, size_t c) {
    block_start_ = b;
    filesize_ = c;
  }
};

// opened file meta
struct FileStatus {
  std::string filename_;
  size_t lba_;              // current block number
  size_t offset_;           // current block offset
  size_t mode_;
  
  FileStatus(std::string filename) {
    filename_ = filename;
    lba_ = 0;
    offset_ = 0;
    mode_ = 0;
  }
};

class FileSystem {
public:
  FileSystem(FlashResult *flashresult);
  
  ~FileSystem();

  void Open(const std::string filename, const size_t mode);

  void Create(const std::string filename);

  void Seek(const std::string filename, const size_t offset);

  void SetFileSize(const std::string filename, const size_t file_size);

  std::string Read(const std::string filename, const size_t read_size);

  void Write(const std::string filename, const char* data, const size_t write_size);

  void Delete(const std::string filename);

  void Close(const std::string filename);
private:
  std::vector<FileStatus> file_buffer_;     // opened files
  std::map<std::string, FCB> fcbs_;              // all files
  size_t *fat_;
  Flash *flash_;
  std::queue<size_t> free_blocks_;

  size_t AssignFreeBlocks();

  void FreeBlock(const size_t lba);

  int SearchInBuffer(const std::string filename);
};
}

#endif