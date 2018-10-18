#include <map>

namespace bilsmtree {

class LRU2QConfig {
public:
  LRU2QConfig();
  ~LRU2QConfig();

  // size for lru
  const static uint M1 = 1000;
  // size for fifo
  const static uint M2 = 1000;
};

class LRU2Q {
public:
  LRU2Q();

  ~LRU2Q();

  KV Put(const Slice& key, const Slice& value);

  Slice Get(const Slice& key);
private:

  std::map<Slice, int> map_;
  BiList* lru_;
  BiList* fifo_;
};

}