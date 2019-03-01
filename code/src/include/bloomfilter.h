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
    if (bits_ < (size_ * 8)) bits_ = size_ * 8;
    size_t bytes = bits_ / size_;
    if (!improved)
      bytes = bits_;
    array_ = new size_t[bytes + 1];
    for (size_t i = 0; i <= bytes; ++ i)
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
    size_t bytes = bits_ / size_;
    if (!improved)
      bytes = bits_;
    array_ = new size_t[bytes + 1];
    for (size_t i = 0; i <= bytes; ++ i) {
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
      const size_t pos = bitpos / size_;
      bool exist = (array_[pos] & (1 << (bitpos % size_)));
      if (!improved) exist = (array_[bitpos] & 1);
      if (!exist)
          return false;
      h += delta_;
    }
    return true;
  }

  virtual std::string ToString() {
    std::stringstream ss;
    ss << bits_;
    ss << Config::DATA_SEG;
    size_t bytes = bits_ / size_;
    if (!improved)
      bytes = bits_;
    for (size_t i = 0; i <= bytes; ++ i) {
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
  const bool improved = false;

  void Add(const Slice key) {
    size_t h = Hash(key.data(), key.size(), Config::FilterConfig::SEED);
    const size_t delta_ = (h >> 34) | (h << 30);
    for (size_t j = 0; j < k_; ++ j) {
      const size_t bitpos = h % bits_;
      const size_t pos = bitpos / size_;
      assert(pos <= (bits_ / size_));
      if (improved)
        array_[pos] |= (1 << (bitpos % size_));
      else
        array_[bitpos] |= 1;
      h += delta_;
    }  
  }

};
}

#endif