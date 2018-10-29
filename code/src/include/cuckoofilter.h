namespace bilsmtree {

class CuckooFilterConfig {
public:
  CuckooFilterConfig();
  ~CuckooFilterConfig();
  
  const static size_t MAXBUCKETSIZE = 4;
};

class Bucket{
public:
  Bucket() { }
 
  Bucket(const size_t max_bits);

  ~Bucket();

  void Push(const Slice& key);

  void Delete(const Slice& key);

private:
  std::vector<Slice> data_;
  size_t max_bits_;
};

class CuckooFilter : Filter {
public:
  CuckooFilter() { }

  CuckooFilter(const std::vector<Slice>& keys);

  ~CuckooFilter();

  void Delete(const Slice& key);

  void Diff(const CuckooFilter& cuckoofilter);
private:
  Bucket* array_;
  size_t capacity_;


  Slice GetFingerPrint(const Slice& key);

  void Add(const Slice& key);
};
}