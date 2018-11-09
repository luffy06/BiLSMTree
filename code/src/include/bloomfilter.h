#ifndef BILSMTREE_BLOOMFILTER_H
#define BILSMTREE_BLOOMFILTER_H

#include <vector>
#include <string>
#include <sstream>

#include "filter.h"

namespace bilsmtree {

class Filter;

class BloomFilter : public Filter {
public:
  BloomFilter(const std::vector<Slice>& keys) {
    k_ = static_cast<uint>(Config::FilterConfig::BITS_PER_KEY * 0.69);
    if (k_ < 1) k_ = 1;
    if (k_ > 30) k_ = 30;

    bits_ = keys.size() * Config::FilterConfig::BITS_PER_KEY;
    if (bits_ < 64) bits_ = 64;
    uint bytes = bits_ / 8;
    array_ = new char[bytes];
    for (uint i = 0; i < keys.size(); ++ i)
      Add(keys[i]);
  }

  BloomFilter(std::string data) {
    k_ = static_cast<uint>(Config::FilterConfig::BITS_PER_KEY * 0.69);
    if (k_ < 1) k_ = 1;
    if (k_ > 30) k_ = 30;

    std::istringstream is(data);
    char temp[100];
    is.read(temp, sizeof(uint));
    bits_ = Util::StringToInt(std::string(temp));
    uint bytes = bits_ / 8;
    array_ = new char[bytes];
    is.read(array_, bytes);
  }
  
  ~BloomFilter() {
    delete array_;
  }

  virtual bool KeyMatch(const Slice key) {
    uint h = Hash(key.data(), key.size(), Config::FilterConfig::SEED);
    const uint delta_ = (h >> 17) | (h << 15);
    for (uint j = 0; j < k_; ++ j) {
      const uint bitpos = h % bits_;
      if ((array_[bitpos / 8] & (1 << (bitpos % 8))) == 0)
        return false;
      h += delta_;
    }
    return true;
  }

  virtual std::string ToString() {
    std::string data = Util::IntToString(bits_);
    data = data + std::string(array_);
    return data;
  }


private:
  uint k_;
  char *array_;
  uint bits_;

  void Add(const Slice key) {
    uint h = Hash(key.data(), key.size(), Config::FilterConfig::SEED);
    const uint delta_ = (h >> 17) | (h << 15);
    for (uint j = 0; j < k_; ++ j) {
      const uint bitpos = h % bits_;
      array_[bitpos / 8] |= (1 << (bitpos % 8));
      h += delta_;
    }  
  }

};
}

#endif