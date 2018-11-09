#ifndef BILSMTREE_SLICE_H
#define BILSMTREE_SLICE_H

#include <string>

namespace bilsmtree {

class Slice {
public:
  Slice() : data_(""), size_(0) { }

  Slice(const char* d, uint n) : data_(d), size_(n) { }

  Slice(const std::string s) : data_(s.data()), size_(s.size()) { }

  Slice(const char* s) : data_(s), size_(strlen(s)) { }

  Slice(const Slice&) = default;
  Slice& operator=(const Slice&) = default;

  const char* data() const { return data_; }

  uint size() const { return size_; }

  bool empty() const { return size_ == 0; }

  char operator[](uint n) const {
    assert(n < size_);
    return data_[n];
  }

  void clear() { data_ = ""; size_ = 0; }

  std::string ToString() const { return std::string(data_, size_); }

  // <  0 iff this <  b
  // == 0 iff this == b
  // >  0 iff this >  b
  int compare(const Slice b) const;

private:
  const char *data_;
  uint size_;
};

struct KV {
  Slice key_;
  Slice value_;

  KV() { }

  KV(const std::string key, const std::string value) : key_(key), value_(value) {  }

  KV(const Slice key, const Slice value) : key_(key), value_(value) {  }

  uint size() const { return key_.size() + value_.size(); }
};

inline bool operator==(const Slice& x, const Slice& y) {
  return ((x.size() == y.size()) &&
          (memcmp(x.data(), y.data(), x.size()) == 0));
}

inline bool operator!=(const Slice& x, const Slice& y) {
  return !(x == y);
}


}

#endif