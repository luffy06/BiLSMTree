namespace bilsmtree {

class DB {
public:
  DB();
  ~DB();

  void Put(const Slice& key, const Slice& value);

  Slice Get(const Slice& key);
private:
  CacherServer* cacheserver_;
  KVServer* kvserver_;
};

}