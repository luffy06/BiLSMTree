#ifndef BILSMTREE_LRU2Q_H
#define BILSMTREE_LRU2Q_H

#include <map>

namespace bilsmtree {

class Slice;
struct KV;
class BiList;

class LRU2QConfig {
public:
  LRU2QConfig();
  ~LRU2QConfig();

  // size for lru
  const static size_t M1 = 1000;
  // size for fifo
  const static size_t M2 = 1000;
};

class LRU2Q {
public:
  LRU2Q();

  ~LRU2Q();

  KV Put(const KV& kv);

  Slice Get(const Slice& key);
private:

  std::map<Slice, size_t> map_;
  BiList *lru_;
  BiList *fifo_;
};

}

#endif