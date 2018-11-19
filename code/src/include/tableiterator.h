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

  TableIterator(const std::string filename, FileSystem* filesystem, Meta meta, LSMTreeResult *lsmtreeresult_);

  ~TableIterator();
  
  void ParseBlock(const std::string block_data);

  void ResetIter() { iter_ = 0; }

  bool HasNext() { return iter_ < kvs_.size(); }

  size_t Id() const { return id_;}

  KV Next() { return kvs_[iter_++]; }

  KV Current() const {
    return kvs_[iter_];
  }

  void SetId(size_t id) { id_ = id; }

  friend bool operator<(const TableIterator& a, const TableIterator& b) {
    KV kv_a = a.Current();
    KV kv_b = b.Current();
    if (kv_a.key_.compare(kv_b.key_) != 0)
      return kv_a.key_.compare(kv_b.key_) > 0;
    return a.Id() > b.Id();
  }
private:
  size_t id_;
  std::vector<KV> kvs_;
  size_t iter_;
  Filter *filter_;
};
}

#endif