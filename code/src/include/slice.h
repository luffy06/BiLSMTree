#ifndef BILSMTREE_SLICE_H
#define BILSMTREE_SLICE_H

#include <string>

namespace bilsmtree {

struct KV {
  Slice key_;
  Slice value_;

  KV() { }

  KV(Slice& key, Slice& value) : key_(key), value_(value) {  }

  size_t size() { return key_.size() + value_.size(); }
};

class Slice {
public:
  Slice() : data_(""), size_(0) { }

  Slice(const char* d, size_t n) : data_(d), size_(n) { }

  Slice(const std::string& s) : data_(s.data()), size_(s.size()) { }

  Slice(const char* s) : data_(s), size_(strlen(s)) { }

  Slice(const Slice&) = default;

  Slice& operator=(const Slice&) = default;

  const char* data() const { return data_; }

  size_t size() const { return size_; }

  bool empty() const { return size_ == 0; }

  char operator[](size_t n) const {
    assert(n < size());
    return data_[n];
  }

  void clear() { data_ = ""; size_ = 0; }

  std::string ToString() const { return std::string(data_, size_); }

  // <  0 iff this <  b
  // == 0 iff this == b
  // >  0 iff this >  b
  int compare(const Slice& b) const;

private:
  const char* data_;
  size_t size_;
};

inline int Slice::compare(const Slice& b) const {
  const size_t min_len = (size_ < b.size_) ? size_ : b.size_;
  int r = memcmp(data_, b.data_, min_len);
  if (r == 0) {
    if (size_ < b.size_) r = -1;
    else if (size_ > b.size_) r = +1;
  }
  return r;
}


}

#endif