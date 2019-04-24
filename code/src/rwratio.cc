#include "rwratio.h"

namespace bilsmtree {

RWRatio::RWRatio(size_t max_size) {
  max_size_ = max_size;
  size_ = 0;
  read_times_ = write_times_ = 0;
}

RWRatio::~RWRatio() {
  max_size_ = size_ = 0;
}

void RWRatio::Append(size_t op) {
  if (op == 0)
    read_times_ = read_times_ + 1;
  else
    write_times_ = write_times_ + 1;
  if (size_ < max_size_) {
    visit_[0].push(op);
  }
  else {
    visit_[0].push(op);
    int res = visit_[0].front();
    visit_[0].pop();
    if (res == 0)
      read_times_ = read_times_ - 1;
    else
      write_times_ = write_times_ - 1;
  }
  size_ ++;
}

double RWRatio::GetReadRatio() {
  return (read_times_ * 1.0) / (read_times_ + write_times_);
}

double RWRatio::GetWriteRatio() {
  return (write_times_ * 1.0) / (read_times_ + write_times_);
}

}