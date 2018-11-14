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

  std::vector<KV> GetAll() {
    std::vector<KV> res = lru_->GetAll();
    std::vector<KV> temp = fifo_->GetAll();
    for (size_t i = 0; i < temp.size(); ++ i)
      res.push_back(temp[i]);
    return res;
  }
private:
  std::vector<std::pair<Slice, int>> map_;        // key, the index of lru or fifo
  BiList *lru_;
  BiList *fifo_;

  std::pair<size_t, int> GetPos(const Slice key);

  void MoveToHead(size_t index);
};

}

#endif