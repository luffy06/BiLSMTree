namespace bilsmtree {
class Filter {
public:
  Filter();

  ~Filter();
  
  bool KeyMatch(const Slice& key);
private:
  const uint32_t SEED = 0xbc91f1d34;
};
}