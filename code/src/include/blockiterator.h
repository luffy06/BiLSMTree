#ifndef BILSMTREE_BLOCKITERATOR_H
#define BILSMTREE_BLOCKITERATOR_H

#include <vector>
#include <string>
#include <sstream>

#include "indextable.h"

namespace bilsmtree {

class BlockIterator {
public:
  BlockIterator();

  BlockIterator(size_t id, BlockMeta bm, std::string block_data);

  ~BlockIterator();
  
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

  void SetId(size_t id) { id_ = id; }

  size_t DataSize() { return kvs_.size(); }
private:
  size_t id_;
  std::vector<KV> kvs_;
  size_t iter_;
};


struct BlockComparator {
  bool operator()(BlockIterator *&a, BlockIterator *&b) const {
    KV kv_a = a->Current();
    KV kv_b = b->Current();
    if (kv_a.key_.compare(kv_b.key_) != 0)
      return kv_a.key_.compare(kv_b.key_) > 0;
    return a->Id() > b->Id();
  }  
};
}

#endif