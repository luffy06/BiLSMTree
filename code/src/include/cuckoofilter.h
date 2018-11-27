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
    data_ = new Slice[Config::CuckooFilterConfig::MAXBUCKETSIZE];
    size_ = 0;
  }
 
  ~Bucket() {
    delete[] data_;
  }

  bool Insert(const Slice fp) {
    if (size_ < Config::CuckooFilterConfig::MAXBUCKETSIZE) {
      data_[size_] = fp;
      size_t i = size_;
      while (i > 0 && data_[i].compare(data_[i - 1]) < 0) {
        Slice temp = data_[i];
        data_[i] = data_[i - 1];
        data_[i - 1] = temp;
        -- i;
      }
      size_ = size_ + 1;
      return true;
    }
    return false;
  }

  Slice Kick(const Slice fp) {
    size_t p = rand() % size_;
    Slice res = data_[p];
    data_[p] = fp;
    size_t i = p;
    if (i > 0 && data_[i].compare(data_[i - 1]) < 0) {
      while (i > 0 && data_[i].compare(data_[i - 1]) < 0) {
        Slice temp = data_[i];
        data_[i] = data_[i - 1];
        data_[i - 1] = temp;
        -- i;
      }
    }
    else if (i < size_ - 1 && data_[i].compare(data_[i + 1]) > 0) {
      while (i < size_ - 1 && data_[i].compare(data_[i + 1]) > 0) {
        Slice temp = data_[i];
        data_[i] = data_[i + 1];
        data_[i + 1] = temp;
        ++ i;
      }      
    }
    return res;
  }

  bool Find(const Slice fp) {
    for (size_t i = 0; i < size_; ++ i) {
      // std::cout << "In Bucket:" << data_[i].ToString() << std::endl;
      if (data_[i].compare(fp) == 0)
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
    data_ = new Slice[Config::CuckooFilterConfig::MAXBUCKETSIZE];
    std::stringstream ss;
    ss.str(data);
    size_ = size;
    for (size_t i = 0; i < size_; ++ i) {
      std::string d;
      ss >> d;
      data_[i] = Slice(d.data(), d.size());
    }
  }

  Slice* GetData() {
    return data_;
  }

  void DeleteDatas(const std::vector<Slice>& deleted_datas) {
    for (size_t i = 0; i < deleted_datas.size(); ++ i) {
      bool found = false;
      for (size_t j = 0; j < size_; ++ j) {
        if (data_[j].compare(deleted_datas[i]) == 0)
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
      ss << data_[i].ToString();
      ss << Config::DATA_SEG;
    }
    return ss.str();
  }
private:
  Slice* data_;
  size_t size_;
};

class CuckooFilter : public Filter {
public:
  CuckooFilter() { }

  CuckooFilter(const std::vector<Slice>& keys) {
    array_ = new Bucket[Config::FilterConfig::CUCKOOFILTER_SIZE];
    data_size_ = 0;
    for (size_t i = 0; i < keys.size(); ++ i)
      Add(keys[i]);
  }

  CuckooFilter(const std::string data) {
    // std::cout << data << std::endl;
    std::stringstream ss;
    ss.str(data);
    ss >> data_size_;
    array_ = new Bucket[Config::FilterConfig::CUCKOOFILTER_SIZE];
    for (size_t i = 0; i < Config::FilterConfig::CUCKOOFILTER_SIZE; ++ i) {
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
    delete[] array_;
  }

  virtual bool KeyMatch(const Slice key) {
    Info info = GetInfo(key);
    // std::cout << "Info of Key:" << key.ToString() << std::endl;
    // std::cout << "FP:" << info.fp_.ToString() << "\tPOS1:" << info.pos1 << "\tPOS2:" << info.pos2 << std::endl;
    return array_[info.pos1].Find(info.fp_) || array_[info.pos2].Find(info.fp_);  
  }

  // this - cuckoofilter
  void Diff(CuckooFilter* cuckoofilter) {
    for (size_t i = 0; i < Config::FilterConfig::CUCKOOFILTER_SIZE; ++ i) {
      Slice* data_ = array_[i].GetData();
      size_t size_ = array_[i].GetSize();
      std::vector<Slice> deleted_datas;
      for (size_t j = 0; j < size_; ++ j) {
        if (cuckoofilter->FindFingerPrint(data_[j], i))
          deleted_datas.push_back(data_[j]);
      }
      array_[i].DeleteDatas(deleted_datas);
    }
  }

  bool FindFingerPrint(const Slice fp, const size_t pos) {
    if (array_[pos].Find(fp))
      return true;
    size_t alt_pos = GetAlternatePos(fp, pos);
    return array_[alt_pos].Find(fp);
  }

  virtual std::string ToString() {
    std::stringstream ss;
    ss << data_size_;
    ss << Config::DATA_SEG;
    for (size_t i = 0; i < Config::FilterConfig::CUCKOOFILTER_SIZE; ++ i)
      ss << array_[i].ToString();
    return ss.str();
  }

private:
  struct Info {
    Slice fp_;
    size_t pos1, pos2;

    Info() { }

    Info(Slice a, size_t b, size_t c) { fp_ = a; pos1 = b; pos2 = c;}
  };

  Bucket *array_;
  // size_t capacity_;
  size_t data_size_;

  const size_t FPSEED = 0xc6a4a793;
  const size_t MAXKICK = 500;

  Slice GetFingerPrint(const Slice key) {
    size_t f = Hash(key.data(), key.size(), FPSEED);
    std::string f_str = Util::IntToString(f);
    // std::cout << "Key:" << key.ToString() << "\tFP:" << f << std::endl;
    return Slice(f_str.data(), f_str.size());
  }

  Info GetInfo(const Slice key) {
    Slice fp_ = GetFingerPrint(key);
    // std::cout << "Key:" << key.ToString() << "\tFP:" << fp_.ToString() << std::endl;
    size_t p1 = Hash(key.data(), key.size(), Config::FilterConfig::SEED) % Config::FilterConfig::CUCKOOFILTER_SIZE;
    size_t p2 = GetAlternatePos(fp_, p1);
    assert(p1 != p2);
    return Info(fp_, p1, p2);
  }

  size_t GetAlternatePos(const Slice fp, const size_t p) {
    size_t hash = Hash(fp.data(), fp.size(), Config::FilterConfig::SEED);
    size_t alt_pos = (p ^ hash) % Config::FilterConfig::CUCKOOFILTER_SIZE;
    // std::cout << "FP:" << fp.ToString() << "\tPOS:" << p << "\tALTER POS:" << alt_pos << "\t" << ((alt_pos ^ hash) % Config::FilterConfig::CUCKOOFILTER_SIZE) << std::endl;
    return alt_pos;
  }

  void InsertAndKick(const Slice fp, const size_t pos) {
    Slice fp_ = fp;
    size_t pos_ = pos;
    for (size_t i = 0; i < MAXKICK; ++ i) {
      fp_ = array_[pos_].Kick(fp_);
      pos_ = GetAlternatePos(fp_, pos_);
      if (array_[pos_].Insert(fp_))
        return ;
    }
    assert(false);
  }

  void Add(const Slice key) {
    Info info = GetInfo(key);
    // std::cout << "Add " << key.ToString() << "\tFP:" << info.fp_.ToString() << "\tPOS1:" << info.pos1 << "\tPOS2:" << info.pos2 << std::endl;
    data_size_ = data_size_ + 1;
    if (array_[info.pos1].Insert(info.fp_) || array_[info.pos2].Insert(info.fp_))
      return ;
    InsertAndKick(info.fp_, rand() % 2 == 0 ? info.pos1 : info.pos2);
  }
};
}

#endif