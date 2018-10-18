#include <vector>

namespace bilsmtree {

class CacheServer {
public:
  CacheServer();
  ~CacheServer();

  void Put(const Slice& key, )
private:
  LRU2Q* lru_;
  std::vector<SkipList*> imms_;
};
}