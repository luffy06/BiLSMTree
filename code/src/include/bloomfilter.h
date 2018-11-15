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
    if (bits_ < 64) bits_ = 64;
    size_t bytes = bits_ / 8;
    array_ = new char[bytes];
    for (size_t i = 0; i < keys.size(); ++ i)
      Add(keys[i]);
  }

  BloomFilter(std::string data) {
    k_ = static_cast<size_t>(Config::FilterConfig::BITS_PER_KEY * 0.69);
    if (k_ < 1) k_ = 1;
    if (k_ > 30) k_ = 30;

    std::stringstream ss;
    ss.str(data);
    ss.read((char *)&bits_, sizeof(size_t));
    size_t bytes = bits_ / 8;
    array_ = new char[bytes];
    ss.read(array_, sizeof(char) * bytes);
  }
  
  ~BloomFilter() {
    delete array_;
  }

  virtual bool KeyMatch(const Slice key) {
    size_t h = Hash(key.data(), key.size(), Config::FilterConfig::SEED);
    const size_t delta_ = (h >> 17) | (h << 15);
    for (size_t j = 0; j < k_; ++ j) {
      const size_t bitpos = h % bits_;
      if ((array_[bitpos / 8] & (1 << (bitpos % 8))) == 0)
        return false;
      h += delta_;
    }
    return true;
  }

  virtual std::string ToString() {
    std::stringstream ss;
    size_t bytes = bits_ / 8;
    ss.write((char *)&bits_, sizeof(size_t));
    ss.write(array_, sizeof(char) * bytes);
    return ss.str();
  }


private:
  size_t k_;
  char *array_;
  size_t bits_;

  void Add(const Slice key) {
    size_t h = Hash(key.data(), key.size(), Config::FilterConfig::SEED);
    const size_t delta_ = (h >> 17) | (h << 15);
    for (size_t j = 0; j < k_; ++ j) {
      const size_t bitpos = h % bits_;
      array_[bitpos / 8] |= (1 << (bitpos % 8));
      h += delta_;
    }  
  }

};
}

#endif