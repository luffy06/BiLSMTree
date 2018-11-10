#ifndef BILSMTREE_LRU2Q_H
#define BILSMTREE_LRU2Q_H

#include <vector>

#include "bilist.h"

namespace bilsmtree {

class LRU2Q {
public:
  LRU2Q();

  ~LRU2Q();

  bool Put(const KV kv, KV& pop_kv);

  bool Get(const Slice key, Slice& value);
private:
  std::vector<std::pair<Slice, int>> map_;
  BiList *lru_;
  BiList *fifo_;

  std::pair<size_t, int> GetPos(const Slice key);

  void MoveToHead(size_t index);
};

}

#endif