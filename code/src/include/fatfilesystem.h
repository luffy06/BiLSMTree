#include <iostream>
#include <fstream>
#include <queue>

#include "flash.h"

class FatFileSystem {
public:
  FatFileSystem();
  ~FatFileSystem();

  int Open(const std::string &filename, const int &mode);

  void Read(const int &file_number, std::string &data, const int &read_size);

  void Write(const int &file_number, const std::string &data, const int &write_size);

  void Close(const int &file_number);

  // void Seek(const int &file_number, const int &offset);

  // int Tell();
  
private:
  const int BLOCK_SIZE = 16;
  const int MAX_BLOCK_NUMS = (1 << 16) * 4;
  const int CLUSTER_NUMS = 64;
  const std::string FILE_META_PATH = "../logs/filesystem.txt";
  const int MAX_FILE_OPEN = 1000;

  struct FCB {
    std::string filename;
    int block_start;
    int filesize; // the number of blocks

    FCB(std::string a, int b, int c) {
      filename = a;
      block_start = b;
      filesize = c;
    }
  };

  struct FileStatus {
    int file_number;
    int lba;
    int mode;
    
    FileStatus() {
      file_number = -1;
      lba = -1;
      mode = -1;
    }
  };

  std::vector<FileStatus> file_buffer;
  std::vector<FCB> fcbs;
  int *fat;
  int open_number;
  Flash *flash;
  std::queue<int> free_blocks;

  int AssignFreeBlocks();

  void FreeBlock(const int &lba);

  int BinarySearchInBuffer(const int &file_number);

  static bool CmpByFileNumber(const FileStatus &fs1, const FileStatus &fs2);
};