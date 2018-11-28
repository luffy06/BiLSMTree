#ifndef BILSMTREE_VIRTUALFILESYSTEM_H
#define BILSMTREE_VIRTUALFILESYSTEM_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <algorithm>
#include <fstream>

#include "filesystem.h"

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

class VirtualFileSystem : public FileSystem {
public:
  VirtualFileSystem();

  ~VirtualFileSystem();

  void Create(const std::string filename);

  size_t Open(const std::string filename, const size_t mode);

  void Seek(const size_t file_number, const size_t offset);

  void SetFileSize(const size_t file_number, const size_t file_size);

  std::string Read(const size_t file_number, const size_t read_size);

  void Write(const size_t file_number, const char* data, const size_t write_size);

  void Delete(const std::string filename);

  void Close(const size_t file_number);
private:
  std::vector<FileStatus> file_buffer_;     // opened files
};

}
#endif