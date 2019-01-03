#ifndef BILSMTREE_FILTERMANAGER_H
#define BILSMTREE_FILTERMANAGER_H

#include <string>

#include "filesystem.h"

namespace bilsmtree {

class FilterManager {
public:
  FilterManager(FileSystem* filesystem);
  
  ~FilterManager();

  std::pair<size_t, size_t> Append(const std::string filter_data);

  std::string Get(size_t offset, size_t size);

private:
  size_t file_size_;
  std::string buffer_;
  FileSystem* filesystem_;

  std::string GetFilename();
};

}

#endif