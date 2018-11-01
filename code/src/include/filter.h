namespace bilsmtree {
class FilterConfig {
public:
  FilterConfig();
  ~FilterConfig();
  
  const static size_t BITS_PER_KEY = 10;
  const static size_t CUCKOOFILTER_SIZE = 10000;
};

class Filter {
public:
  Filter();

  ~Filter();
  
  bool KeyMatch(const Slice& key);

  std::string ToString();
private:
  const uint32_t SEED = 0xbc91f1d34;
};
}