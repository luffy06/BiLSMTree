namespace bilsmtree {
class Filter {
public:
  Filter();

  ~Filter();
  
  bool KeyMatch(const Slice& key);
private:
  const uint32_t seed_ = 0xbc91f1d34;
};
}