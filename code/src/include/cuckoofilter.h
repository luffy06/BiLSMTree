namespace bilsmtree {

class CuckooFilterConfig {
public:
  CuckooFilterConfig();
  ~CuckooFilterConfig();
  
  const static size_t MAXBUCKETSIZE = 4;
};

class Bucket{
public:
  Bucket();
 
  ~Bucket();

  bool Insert(const Slice& key);

  Slice Kick(const Slice& key);

  bool Delete(const Slice& key);

  bool Find(const Slice& key);

  void Diff(const uint32_t& pos, const CuckooFilter* cuckoofilter);
private:
  Slice* data_;
  size_t size_;
};

class CuckooFilter : Filter {
public:
  CuckooFilter() { }

  CuckooFilter(const size_t capacity, const std::vector<Slice>& keys);

  ~CuckooFilter();

  bool Delete(const Slice& key);

  void Diff(const CuckooFilter* cuckoofilter);

  bool FindFingerPrint(const Slice& fp, const uint32_t& pos)
private:
  struct Info {
    Slice fp_;
    uint32_t pos1, pos2;

    Info() { }

    Info(Slice a, uint32_t b, uint32_t c) { fp_ = a; pos1 = b; pos2 = c;}
  };

  Bucket* array_;
  size_t capacity_;
  size_t size_;

  const uint32_t FPSEED = 0xc6a4a793;
  const size_t MAXKICK = 500;

  Slice GetFingerPrint(const Slice& key);

  Info GetInfo(const Slice& key);

  uint32_t GetAlternatePos(const Slice& key, const uint32_t& p);

  void InsertAndKick(const Slice& key, const uint32_t pos);

  void Add(const Slice& key);
};
}