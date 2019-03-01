#ifndef BILSMTREE_TABLEITERATOR_H
#define BILSMTREE_TABLEITERATOR_H

#include <vector>
#include <string>
#include <sstream>

#include "table.h"

namespace bilsmtree {

class TableIterator {
public:
  TableIterator();

  TableIterator(const std::string filename, FileSystem* filesystem, Meta meta, 
                LSMTreeResult *lsmtreeresult_, 
                const std::vector<std::string> &bloom_algos, 
                const std::vector<std::string> &cuckoo_algos);

  ~TableIterator();

  void SetData(const std::vector<KV> &data) {
    kvs_.clear();
    for (size_t i = 0; i < data.size(); ++ i)
      kvs_.push_back(data[i]);
    ResetIter();
  }
  
  void ResetIter() { iter_ = 0; }

  bool HasNext() { return iter_ < kvs_.size(); }

  size_t Id() const { return id_;}

  KV Next() {
    assert(iter_ < kvs_.size());
    return kvs_[iter_++];
  }

  KV Current() const {
    assert(iter_ < kvs_.size());
    return kvs_[iter_];
  }

  KV Max() const {
    return *kvs_.rbegin();
  }

  KV Min() const {
    return *kvs_.begin();
  }

  void SetId(size_t id) { id_ = id; }

  size_t DataSize() { return kvs_.size(); }

private:
  size_t id_;
  std::vector<KV> kvs_;
  size_t iter_;

  void ParseBlock(const std::string block_data, Filter* filter);
};

struct TableComparator {
  bool operator()(TableIterator *&a, TableIterator *&b) const {
    KV kv_a = a->Current();
    KV kv_b = b->Current();
    if (kv_a.key_.compare(kv_b.key_) != 0)
      return kv_a.key_.compare(kv_b.key_) > 0;
    return a->Id() > b->Id();
  }  
};
}

#endif