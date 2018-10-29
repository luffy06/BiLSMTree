namespace bilsmtree {

class BloomFilter : Filter {
public:
  BloomFilter() { }

  BloomFilter(const size_t bits_per_key, const std::vector<Slice>& keys);
  
  ~BloomFilter();

private:
  const size_t bits_per_key_;
  const size_t k_;

  char* array_;
  size_t bits_;

  void Add(const Slice& key);
};
}