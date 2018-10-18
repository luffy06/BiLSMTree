namespace bilsmtree {

class LRU2Q {
public:
  LRU2Q();
  ~LRU2Q();

  void Put(const Slice& key, const Slice& value);

  Slice Get(const Slice& key);
private:
  Slice* fifo_;
};

class LRU2QConfig
{
public:
  LRU2QConfig();
  ~LRU2QConfig();

  // size for lru
  const static uint M1 = 1000;
  // size for fifo
  const static uint M2 = 1000;
};

}