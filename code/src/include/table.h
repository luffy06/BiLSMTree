#ifndef BILSMTREE_TABLE_H
#define BILSMTREE_TABLE_H

#include <vector>
#include <string>

#include "bloomfilter.h"
#include "cuckoofilter.h"
#include "filesystem.h"

namespace bilsmtree {

struct Meta {
  Slice largest_;
  Slice smallest_;
  size_t sequence_number_;
  size_t level_;
  size_t file_size_;
  size_t footer_size_;

  Meta() {
  }

  void Copy(const Meta meta) {
    largest_ = Slice(meta.largest_.data(), meta.largest_.size());
    smallest_ = Slice(meta.smallest_.data(), meta.smallest_.size());
    sequence_number_ = meta.sequence_number_;
    level_ = meta.level_;
    file_size_ = meta.file_size_;
    footer_size_ = meta.footer_size_;
  }

  bool Intersect(Meta other) const {
    if (largest_.compare(other.smallest_) >= 0 && smallest_.compare(other.largest_) <= 0)
      return true;
    return false;
  }

  void Show() const {
    std::cout << std::string(20, '*') << std::endl;
    std::cout << "SEQ:" << sequence_number_ << "\tLevel:" << level_ << std::endl;
    std::cout << "Smallest Size:" << smallest_.size() << std::endl;
    std::cout << "Smallest:" << smallest_.ToString() << std::endl;
    std::cout << "Largest Size:" << largest_.size() << std::endl;
    std::cout << "Largest:" << largest_.ToString() << std::endl;
    std::cout << "FileSize:" << file_size_ << "\tFooterSize:" << footer_size_ << std::endl;
    std::cout << std::string(20, '*') << std::endl;
  }

  friend bool operator<(const Meta& a, const Meta& b) {
    // std::cout << "Compare a:" << a.largest_.size() << std::endl;
    // std::cout << a.largest_.ToString() << std::endl;
    // std::cout << "Compare b:" << b.largest_.size() << std::endl;
    // std::cout << b.largest_.ToString() << std::endl;
    // std::cout << "SEQ:" << a.sequence_number_ << "\t" << b.sequence_number_ << std::endl;
    if (a.largest_.compare(b.largest_) != 0)
      return a.largest_.compare(b.largest_) <= 0;
    return a.smallest_.compare(b.smallest_) <= 0;
  }
};

class Block {
public:
  Block(const char* data, const size_t size) : size_(size) {
    data_ = new char[size_ + 1];
    for (size_t i = 0; i < size_; ++ i)
      data_[i] = data[i];
    data_[size_] = '\0';
  }
  
  ~Block() { }

  const char* data() { return data_; }

  size_t size() { return size_; }
private:
  char *data_;
  size_t size_;
};

class Table {
public:
  Table();

  Table(const std::vector<KV>& kvs, FileSystem* filesystem);
  
  ~Table();

  Meta GetMeta();

  void DumpToFile(const std::string filename, LSMTreeResult* lsmtreeresult);
private:
  Block **data_blocks_;
  Block *index_block_;
  Filter *filter_;
  size_t data_block_number_;
  std::string footer_data_;
  Meta meta_;
  FileSystem* filesystem_;
};


}

#endif