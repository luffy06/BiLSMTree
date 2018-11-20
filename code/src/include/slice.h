#ifndef BILSMTREE_SLICE_H
#define BILSMTREE_SLICE_H

#include <cstring>
#include <cassert>

#include <iostream>
#include <string>

namespace bilsmtree {

class Slice {
public:
  Slice() : size_(0) {
    data_ = new char[2];
    data_[0] = '\0';
  }

  Slice(const char* d, size_t n) : size_(n) {
    data_ = new char[size_ + 1];
    Copy(d);
  }

  Slice(const Slice& s) {
    size_ = s.size();
    data_ = new char[size_ + 1];
    Copy(s.data());
  }

  const char* data() const { return data_; }

  size_t size() const { return size_; }

  bool empty() const { return size_ == 0; }

  char operator[](size_t n) const {
    assert(n < size_);
    return data_[n];
  }

  std::string ToString() const { 
    return std::string(data_, size_); 
  }

  // <  0 iff this <  b
  // == 0 iff this == b
  // >  0 iff this >  b
  int compare(const Slice b) const;

private:
  char *data_;
  size_t size_;

  void Copy(const char* d) {
    for (size_t i = 0; i < size_; ++ i)
      data_[i] = d[i];
    data_[size_] = '\0';
  }
};

struct KV {
  Slice key_;
  Slice value_;

  KV() { }

  KV(const std::string key, const std::string value) {
    key_ = Slice(key.data(), key.size());
    value_ = Slice(value.data(), value.size());
  }

  KV(const Slice key, const Slice value) {
    key_ = Slice(key.data(), key.size());
    value_ = Slice(value.data(), value.size());
  }

  size_t size() const { return key_.size() + value_.size(); }

  void show() const {
    std::cout << "Key\t" << key_.size() << std::endl << key_.ToString() << std::endl;
    std::cout << "Value\t" << value_.size() << std::endl << value_.ToString() << std::endl;
    std::cout << std::endl;
  }
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