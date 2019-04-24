#ifndef BILSMTREE_RWRATIO_H
#define BILSMTREE_RWRATIO_H

#include <queue>

namespace bilsmtree {

class RWRatio {
public:
  RWRatio(size_t max_size);

  ~RWRatio();

  void Append(size_t op);

  double GetReadRatio();

  double GetWriteRatio();

  void Clear();

private:
  std::queue<size_t> visit_[2];

  size_t max_size_;
  size_t size_;
  size_t read_times_;
  size_t write_times_;
};
}

#endif