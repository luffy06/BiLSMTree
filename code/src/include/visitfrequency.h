#ifndef BILSMTREE_VISITFREQUENCY_H
#define BILSMTREE_VISITFREQUENCY_H

#include <queue>

#include "filesystem.h"

namespace bilsmtree {

class VisitFrequency {
public:
  VisitFrequency(size_t max_size, FileSystem* filesystem);

  ~VisitFrequency();

  int Append(size_t file_number);
private:
  std::queue<size_t> visit_[2];
  FileSystem* filesystem_;

  size_t max_size_;
  size_t size_;

  size_t mode_; // 0 for small queue 1 for big queue 

  // void Dump();

  // void Load();
};
}

#endif