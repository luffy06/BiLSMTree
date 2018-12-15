#ifndef BILSMTREE_CUCKOOFILTER_H
#define BILSMTREE_CUCKOOFILTER_H

#include <iostream>
#include <vector>
#include <string>

#include "filter.h"

namespace bilsmtree {

class Bucket{
public:
  Bucket() {
    data_.resize(Config::CuckooFilterConfig::MAXBUCKETSIZE);
    size_ = 0;
  }
 
  ~Bucket() {
  }

  bool Insert(size_t fp) {
    if (size_ < Config::CuckooFilterConfig::MAXBUCKETSIZE) {
      data_[size_] = fp;
      int i = size_;
      while (i > 0 && data_[i] < data_[i - 1]) {
        size_t temp = data_[i];
        data_[i] = data_[i - 1];
        data_[i - 1] = temp;
        -- i;
      }
      size_ = size_ + 1;
      return true;
    }
    return false;
  }

  size_t Kick(const size_t fp) {
    size_t p = rand() % size_;
    size_t res = data_[p];
    data_[p] = fp;
    int i = p;
    if (i > 0 && data_[i] < data_[i - 1]) {
      while (i > 0 && data_[i] < data_[i - 1]) {
        size_t temp = data_[i];
        data_[i] = data_[i - 1];
        data_[i - 1] = temp;
        -- i;
      }
    }
    else if (i < size_ - 1 && data_[i] > data_[i + 1]) {
      while (i < size_ - 1 && data_[i] > data_[i + 1]) {
        size_t temp = data_[i];
        data_[i] = data_[i + 1];
        data_[i + 1] = temp;
        ++ i;
      }      
    }
    return res;
  }

  bool Find(size_t fp) {
    for (size_t i = 0; i < size_; ++ i) {
      if (data_[i] == fp)
        return true;
    }
    return false;
  }

  void SetSize(const size_t s) {
    size_ = s;
  }

  size_t GetSize() {
    return size_;
  }

  void SetData(const std::string data, const size_t size) {
    if (size > Config::CuckooFilterConfig::MAXBUCKETSIZE)
      std::cout << "Error Size:" << size << std::endl;
    assert(size <= Config::CuckooFilterConfig::MAXBUCKETSIZE);
    data_.resize(Config::CuckooFilterConfig::MAXBUCKETSIZE);
    std::stringstream ss;
    ss.str(data);
    size_ = size;
    for (size_t i = 0; i < size_; ++ i)
      ss >> data_[i];
  }

  std::vector<size_t> GetData() {
    return data_;
  }

  void DeleteDatas(const std::vector<size_t>& deleted_datas) {
    for (size_t i = 0; i < deleted_datas.size(); ++ i) {
      bool found = false;
      for (size_t j = 0; j < size_; ++ j) {
        if (data_[j] == deleted_datas[i])
          found = true;
        if (found && j < size_ - 1)
          data_[j] = data_[j + 1];
      }
      assert(found);
      size_ = size_ - 1;
    }
  }

  std::string ToString() {
    std::stringstream ss;
    ss << size_;
    ss << Config::DATA_SEG;
    for (size_t i = 0; i < size_; ++ i) {
      ss << data_[i];
      ss << Config::DATA_SEG;
    }
    return ss.str();
  }
private:
  std::vector<size_t> data_;
  size_t size_;
};

class CuckooFilter : public Filter {
public:
  CuckooFilter() { }

  CuckooFilter(const std::vector<Slice>& keys) {
    data_size_ = keys.size();
    data_size_ = std::max(data_size_, (size_t)2);
    data_size_ = data_size_ * 2;
    max_kick_ = keys.size();
    array_.resize(data_size_);
    for (size_t i = 0; i < keys.size(); ++ i)
      Add(keys[i]);
  }

  CuckooFilter(const std::string data) {
    // std::cout << data << std::endl;
    std::stringstream ss;
    ss.str(data);
    ss >> data_size_;
    max_kick_ = data_size_;
    array_.resize(data_size_);
    for (size_t i = 0; i < data_size_; ++ i) {
      size_t array_size_ = 0;
      ss >> array_size_;
      std::string array_data_ = "";
      // if (array_size_ > 0)
      //   std::cout << "Init " << array_size_ << " Array Data" << std::endl;
      for (size_t j = 0; j < array_size_; ++ j) {
        std::string d;
        ss >> d;
        array_data_ = array_data_ + d + "\t";
      }
      // if (array_size_ > 0)
      //   std::cout << "Set " << array_size_ << " Data:" << array_data_ << std::endl;
      array_[i].SetData(array_data_, array_size_);
    }
  }

  ~CuckooFilter() {
  }

  virtual bool KeyMatch(const Slice key) {
    Info info = GetInfo(key);
    return array_[info.pos1].Find(info.fp_) || array_[info.pos2].Find(info.fp_);  
  }

  // this - cuckoofilter
  void Diff(CuckooFilter* cuckoofilter) {
    for (size_t i = 0; i < data_size_; ++ i) {
      std::vector<size_t> data_ = array_[i].GetData();
      size_t size_ = array_[i].GetSize();
      std::vector<size_t> deleted_datas;
      for (size_t j = 0; j < size_; ++ j) {
        if (cuckoofilter->FindFingerPrint(data_[j], i))
          deleted_datas.push_back(data_[j]);
      }
      array_[i].DeleteDatas(deleted_datas);
    }
  }

  bool FindFingerPrint(const size_t fp, const size_t pos) {
    if (pos >= data_size_)
      return false;
    if (array_[pos].Find(fp))
      return true;
    size_t alt_pos = GetAlternatePos(fp, pos);
    return array_[alt_pos].Find(fp);
  }

  virtual std::string ToString() {
    std::stringstream ss;
    ss << data_size_;
    ss << Config::DATA_SEG;
    for (size_t i = 0; i < data_size_; ++ i)
      ss << array_[i].ToString();
    return ss.str();
  }

private:
  struct Info {
    size_t fp_;
    size_t pos1, pos2;

    Info() { }

    Info(size_t a, size_t b, size_t c) { fp_ = a; pos1 = b; pos2 = c;}
  };

  std::vector<Bucket> array_;
  // size_t capacity_;
  size_t data_size_;
  size_t max_kick_;

  const size_t FPSEED = 0xc6a4a793;

  size_t GetFingerPrint(const Slice key) {
    size_t f = Hash(key.data(), key.size(), FPSEED);
    return f;
  }

  Info GetInfo(const Slice key) {
    size_t fp = GetFingerPrint(key);
    size_t p1 = fp % data_size_;
    size_t p2 = (fp + Config::FilterConfig::PADDING) % data_size_;
    if (p1 == p2)
      std::cout << fp << "\t" << p1 << "\t" << p2 << "\t" << data_size_ << std::endl;
    assert(p1 != p2);
    return Info(fp, p1, p2);
  }

  size_t GetAlternatePos(size_t fp, size_t p) {
    size_t p1 = fp % data_size_;
    size_t p2 = (fp + Config::FilterConfig::PADDING) % data_size_;    
    if (p == p1)
      return p2;
    return p1;
  }

  void InsertAndKick(size_t fp, size_t pos) {
    for (size_t i = 0; i < max_kick_; ++ i) {
      fp = array_[pos].Kick(fp);
      pos = GetAlternatePos(fp, pos);
      if (array_[pos].Insert(fp))
        return ;
    }
    assert(false);
  }

  void Add(const Slice key) {
    Info info = GetInfo(key);
    // std::cout << "Add " << key.ToString() << "\tFP:" << info.fp_.ToString() << "\tPOS1:" << info.pos1 << "\tPOS2:" << info.pos2 << std::endl;
    if (array_[info.pos1].Insert(info.fp_) || array_[info.pos2].Insert(info.fp_))
      return ;
    InsertAndKick(info.fp_, rand() % 2 == 0 ? info.pos1 : info.pos2);
  }
};
}

#endif