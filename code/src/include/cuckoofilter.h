#ifndef BILSMTREE_CUCKOOFILTER_H
#define BILSMTREE_CUCKOOFILTER_H

#include <iostream>
#include <vector>
#include <string>
#include <sstream>

#include "filter.h"

namespace bilsmtree {

class Filter;

class Bucket{
public:
  Bucket() {
    data_ = new Slice[Config::CuckooFilterConfig::MAXBUCKETSIZE];
    for (uint i = 0; i < Config::CuckooFilterConfig::MAXBUCKETSIZE; ++ i)
      data_[i] = NULL;
    size_ = 0;
  }
 
  ~Bucket() {
    delete data_;
  }

  bool Insert(const Slice fp) {
    if (size_ < Config::CuckooFilterConfig::MAXBUCKETSIZE) {
      data_[size_] = fp;
      return true;
    }
    return false;
  }

  Slice Kick(const Slice fp) {
    uint i = rand() % size_;
    Slice res = data_[i];
    data_[i] = fp;
    return res;
  }

  bool Delete(const Slice fp) {
    bool found = false;
    for (uint i = 0; i < size_; ++ i) {
      if (found)
        data_[i - 1] = data_[i];
      if (data_[i].compare(fp) == 0)
        found = true;
    }
    if (found)
      size_ -= 1;
    return found;
  }

  bool Find(const Slice fp) {
    for (uint i = 0; i < size_; ++ i)
      if (data_[i].compare(fp) == 0)
        return true;
    return false;
  }

  void SetSize(const uint s) {
    size_ = s;
  }

  uint GetSize() {
    return size_;
  }

  void SetData(const uint pos, const char* temp) {
    data_[pos] = Slice(temp);
  }

  Slice* GetData() {
    return data_;
  }

  void DeleteIndexs(const std::vector<uint>& deleted_indexs) {
    for (uint i = 0, j = 0, k = 0; k < deleted_indexs.size();) {
      while (i < size_ && k < deleted_indexs.size() && i == deleted_indexs[k]) {
        ++ i;
        ++ k;
      }
      data_[j] = Slice(data_[i].data());
      ++ j;
      ++ i;
    }
    size_ = size_ - deleted_indexs.size();
  }

  std::string ToString() {
    std::string data = "";
    for (uint i = 0; i < size_; ++ i) 
      data = data + data_[i].ToString();
    return data;
  }
private:
  Slice* data_;
  uint size_;
};

class CuckooFilter : public Filter {
public:
  CuckooFilter() { }

  CuckooFilter(const uint capacity, const std::vector<Slice>& keys) {
    Util::Assert("CAPACITY IS ZERO", capacity > 0);
    capacity_ = capacity;
    array_ = new Bucket[capacity_];
    size_ = 0;
    for (uint i = 0; i < keys.size(); ++ i)
      Add(keys[i]);
  }

  CuckooFilter(const std::string data) {
    std::istringstream is(data);
    char temp[100];
    is.read(temp, sizeof(uint));
    capacity_ = Util::StringToInt(std::string(temp));
    is.read(temp, sizeof(uint));
    size_ = Util::StringToInt(std::string(temp));
    array_ = new Bucket[capacity_];
    for (uint i = 0; i < size_; ++ i) {
      is.read(temp, sizeof(uint));
      uint size_ = Util::StringToInt(std::string(temp));
      array_[i].SetSize(size_);
      for (uint j = 0; j < size_; ++ j) {
        is.read(temp, sizeof(uint));
        array_[i].SetData(j, temp);
      }
    }
  }

  ~CuckooFilter() {
    delete[] array_;
  }

  bool Delete(const Slice key) {
    Info info = GetInfo(key);
    return array_[info.pos1].Delete(info.fp_) || array_[info.pos2].Delete(info.fp_);
  }

  virtual bool KeyMatch(const Slice key) {
    Info info = GetInfo(key);
    return array_[info.pos1].Find(info.fp_) || array_[info.pos2].Find(info.fp_);  
  }

  void Diff(CuckooFilter* cuckoofilter) {
    for (uint i = 0; i < capacity_; ++ i) {
      Slice* data_ = array_[i].GetData();
      std::vector<uint> deleted_indexs;
      for (uint i = 0; i < size_; ++ i) {
        if (cuckoofilter -> FindFingerPrint(data_[i], i))
          deleted_indexs.push_back(i);
      }
      array_[i].DeleteIndexs(deleted_indexs);
    }
  }

  bool FindFingerPrint(const Slice fp, const uint pos) {
    if (array_[pos].Find(fp))
      return true;
    uint alt_pos = GetAlternatePos(fp, pos);
    return array_[alt_pos].Find(fp);
  }

  virtual std::string ToString() {
    std::string data = Util::IntToString(capacity_) + Util::IntToString(size_);
    for (uint i = 0; i < size_; ++ i) {
      data = data + Util::IntToString(array_[i].GetSize());
      data = data + array_[i].ToString();
    }
    return data;
  }

private:
  struct Info {
    Slice fp_;
    uint pos1, pos2;

    Info() { }

    Info(Slice a, uint b, uint c) { fp_ = a; pos1 = b; pos2 = c;}
  };

  Bucket *array_;
  uint capacity_;
  uint size_;

  const uint FPSEED = 0xc6a4a793;
  const uint MAXKICK = 500;

  Slice GetFingerPrint(const Slice key) {
    uint f = Hash(key.data(), key.size(), FPSEED) % 255 + 1;
    char *str = new char[128];
    sprintf(str, "%zu", f);
    return Slice(std::string(str));
  }

  Info GetInfo(const Slice key) {
    Slice fp_ = GetFingerPrint(key);
    uint p1 = Hash(key.data(), key.size(), Config::FilterConfig::SEED) % capacity_;
    uint p2 = GetAlternatePos(fp_, p1);
    return Info(fp_, p1, p2);  
  }

  uint GetAlternatePos(const Slice fp, const uint p) {
    uint hash = Hash(fp.data(), fp.size(), Config::FilterConfig::SEED);
    return (p ^ hash) % capacity_;
  }

  void InsertAndKick(const Slice fp, const uint pos) {
    Slice fp_ = Slice(fp.data(), fp.size());
    uint pos_ = pos;
    for (uint i = 0; i < MAXKICK; ++ i) {
      fp_ = array_[pos_].Kick(fp_);
      pos_ = GetAlternatePos(fp_, pos_);
      if (array_[pos_].Insert(fp_))
        return ;
    }
    Util::Assert("SPACE IS NOT ENOUGH", false);
  }

  void Add(const Slice key) {
    Info info = GetInfo(key);
    if (array_[info.pos1].Insert(info.fp_) || array_[info.pos2].Insert(info.fp_))
      return ;
    InsertAndKick(info.fp_, rand() % 2 == 0 ? info.pos1 : info.pos2);
    size_ = size_ + 1;
  }
};
}

#endif