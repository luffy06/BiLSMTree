#ifndef BILSMTREE_BLOOMFILTER_H
#define BILSMTREE_BLOOMFILTER_H

#include <vector>
#include <string>
#include <sstream>

#include "filter.h"

namespace bilsmtree {

class BloomFilter : public Filter {
public:
  BloomFilter(const std::vector<Slice>& keys) {
    k_ = static_cast<size_t>(Config::FilterConfig::BITS_PER_KEY * 0.69);
    if (k_ < 1) k_ = 1;
    if (k_ > 30) k_ = 30;

    bits_ = keys.size() * Config::FilterConfig::BITS_PER_KEY;
    array_ = new size_t[bits_ + 1];
    for (size_t i = 0; i <= bits_; ++ i)
      array_[i] = 0;
    for (size_t i = 0; i < keys.size(); ++ i)
      Add(keys[i]);
  }

  BloomFilter(std::string data) {
    k_ = static_cast<size_t>(Config::FilterConfig::BITS_PER_KEY * 0.69);
    if (k_ < 1) k_ = 1;
    if (k_ > 30) k_ = 30;

    std::stringstream ss;
    ss.str(data);
    ss >> bits_;
    array_ = new size_t[bits_ + 1];
    for (size_t i = 0; i <= bits_; ++ i) {
      ss >> array_[i];
    }
  }
  
  ~BloomFilter() {
    delete[] array_;
  }

  virtual bool KeyMatch(const Slice key) {
    size_t h = Hash(key.data(), key.size(), Config::FilterConfig::SEED);
    const size_t delta_ = (h >> 34) | (h << 30);
    for (size_t j = 0; j < k_; ++ j) {
      const size_t bitpos = h % bits_;
      if (!(array_[bitpos] & 1))
        return false;
      h += delta_;
    }
    return true;
  }

  virtual std::string ToString() {
    std::stringstream ss;
    ss << bits_;
    ss << Config::DATA_SEG;
    for (size_t i = 0; i <= bits_; ++ i) {
      ss << array_[i];
      ss << Config::DATA_SEG;
    }
    return ss.str();
  }


private:
  size_t k_;
  size_t *array_;
  size_t bits_;
  const size_t size_ = sizeof(size_t) * 8;

  void Add(const Slice key) {
    size_t h = Hash(key.data(), key.size(), Config::FilterConfig::SEED);
    const size_t delta_ = (h >> 34) | (h << 30);
    for (size_t j = 0; j < k_; ++ j) {
      const size_t bitpos = h % bits_;
      array_[bitpos] |= 1;
      h += delta_;
    }  
  }

};
}

#endif