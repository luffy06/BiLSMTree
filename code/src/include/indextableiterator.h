#ifndef BILSMTREE_INDEXTABLEITERATOR_H
#define BILSMTREE_INDEXTABLEITERATOR_H

#include <vector>
#include <string>
#include <sstream>

#include "table.h"
namespace bilsmtree {

class IndexTableIterator {
public:
  IndexTableIterator();

  IndexTableIterator(const std::string filename, FileSystem* filesystem, Meta meta, LSMTreeResult *lsmtreeresult_);

  ~IndexTableIterator();
  
  void ResetIter() { iter_ = 0; }

  bool HasNext() { return iter_ < bms_.size(); }

  size_t Id() const { return id_;}

  KV Next() {
    assert(iter_ < bms_.size());
    return bms_[iter_++];
  }

  BlockMeta Current() const {
    assert(iter_ < bms_.size());
    return bms_[iter_];
  }

  void SetId(size_t id) { id_ = id; }

  size_t DataSize() { return bms_.size(); }
private:
  size_t id_;
  std::vector<BlockMeta> bms_;
  size_t iter_;
};

struct IndexTableComparator {
  bool operator()(TableIterator *&a, TableIterator *&b) const {
    BlockMeta bm_a = a->Current();
    BlockMeta bm_b = b->Current();
    if (bm_a.smallest_.compare(bm_b.smallest_) != 0)
      return bm_a.smallest_.compare(bm_b.smallest_) > 0;
    return a->Id() > b->Id();
  }  
};
}

#endif