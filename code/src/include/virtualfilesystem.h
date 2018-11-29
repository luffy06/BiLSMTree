#ifndef BILSMTREE_VIRTUALFILESYSTEM_H
#define BILSMTREE_VIRTUALFILESYSTEM_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <algorithm>
#include <fstream>

#include "flash.h"

/* FORMAT
* FILENAME  CREATE
* FILENAME  OPEN  MODE
* FILENAME  SEEK  OFFSET
* FILENAME  READ  OFFSET  READSIZE
* FILENAME  WRITE OFFSET  WRITESIZE
* FILENAME  DELETE
* FILENAME  CLOSE
*/

namespace bilsmtree {

// opened file meta
struct FileStatus {
  std::string filename_;
  size_t mode_;
  size_t offset_;
  size_t filesize_;
  
  FileStatus(std::string filename, size_t mode) {
    filename_ = filename;
    mode_ = mode;
    std::fstream f;
    f.open(filename, std::ios::in | std::ios::out);
    f.seekg(0, f.end);
    if (f.tellg() == -1)
      filesize_ = 0;
    else
      filesize_ = f.tellg();
    f.close();
    offset_ = 0;
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

  int SearchInBuffer(const std::string filename);
};

}
#endif