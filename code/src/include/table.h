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
  size_t data_numb_;

  Meta() {
  }

  void Copy(const Meta meta) {
    largest_ = meta.largest_;
    smallest_ = meta.smallest_;
    sequence_number_ = meta.sequence_number_;
    level_ = meta.level_;
    file_size_ = meta.file_size_;
    footer_size_ = meta.footer_size_;
    data_numb_ = meta.data_numb_;
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
    if (a.largest_.compare(b.largest_) != 0)
      return a.largest_.compare(b.largest_) <= 0;
    return a.smallest_.compare(b.smallest_) <= 0;
  }
};

class Table {
public:
  Table();

  Table(const std::vector<KV>& kvs, size_t sequence_number, std::string filename, 
        FileSystem* filesystem, LSMTreeResult* lsmtreeresult, const std::vector<std::string> &bloom_algos, 
        const std::vector<std::string> &cuckoo_algos);
  
  ~Table();

  Meta GetMeta();

private:
  Meta meta_;
};


}

#endif