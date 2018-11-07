#ifndef BILSMTREE_DB_H
#define BILSMTREE_DB_H

namespace bilsmtree {

class Slice;
struct KV;
class CacheServer;
class KVServer;

class DB {
public:
  DB();
  ~DB();

  void Put(const Slice& key, const Slice& value);

  Slice Get(const Slice& key);
private:
  CacherServer *cacheserver_;
  KVServer *kvserver_;
};

}

#endif