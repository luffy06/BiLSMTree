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
  uint block_start_;      // the start of block
  uint filesize_;         // the number of blocks

  FCB() {
    std::cout << "EMPTY FCB" << std::endl;
  }

  FCB(const std::string& a, uint b, uint c) {
    filename_ = a;
    block_start_ = b;
    filesize_ = c;
  }
};

struct FileStatus {
  uint file_number_;
  uint lba_;              // current block number
  uint offset_;           // current block offset
  uint mode_;
  
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

  static uint Open(const std::string filename, const uint mode);

  static void Seek(const uint file_number, const uint offset);

  static uint GetFileSize(const uint file_number);

  static std::string Read(const uint file_number, const uint read_size);

  static void Write(const uint file_number, const std::string data, const uint write_size);

  static void Delete(const std::string filename);

  static void Close(const uint file_number);  
private:
  static std::vector<FileStatus> file_buffer_;
  static std::map<uint, FCB> fcbs_;
  static uint *fat_;
  static uint total_file_number_;
  static uint open_number_;
  static Flash *flash_;
  static std::queue<uint> free_blocks_;

  static uint AssignFreeBlocks();

  static void FreeBlock(const uint lba);

  static int BinarySearchInBuffer(const uint file_number);
};
}

#endif