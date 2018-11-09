#ifndef BILSMTREE_VISITFREQUENCY_H
#define BILSMTREE_VISITFREQUENCY_H

#include <queue>

#include "filesystem.h"

namespace bilsmtree {

class FileSystem;

class VisitFrequency {
public:
  VisitFrequency() { }

  VisitFrequency(uint max_size);

  ~VisitFrequency();

  int Append(uint file_number);
private:
  std::queue<uint> visit_[2];

  uint max_size_;
  uint size_;

  uint mode_; // 0 for small queue 1 for big queue 

  void Dump();

  void Load();
};
}

#endif