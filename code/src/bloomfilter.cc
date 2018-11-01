namespace bilsmtree {

BloomFilter::BloomFilter(const std::vector<Slice>& keys) {
  k_ = static_cast<size_t>(FilterConfig::BITS_PER_KEY * 0.69);
  if (k_ < 1) k_ = 1;
  if (k_ > 30) k_ = 30;

  bits_ = keys.size() * FilterConfig::BITS_PER_KEY;
  if (bits_ < 64) bits_ = 64;
  size_t bytes = bits_ / 8;
  array_ = new char[bytes];
  for (size_t i = 0; i < keys.size(); ++ i)
    Add(keys[i]);
}

BloomFilter::BloomFilter(std::string data) {
  std::istringstream is(data);
  char temp[100];
  is.read(temp, sizeof(size_t));
  bits_ = Util::StringToLong(std::string(temp));
  size_t bytes = bits_ / 8;
  array_ = new char[bytes];
  is.read(array_, bytes);
}

BloomFilter::~BloomFilter() {
  delete array_;
}


bool BloomFilter::KeyMatch(const Slice& key) {
  uint32_t h = Hash(key.data(), key.size(), SEED);
  const uint32_t delta = (h >> 17) | (h << 15);
  for (size_t j = 0; j < k_; ++ j) {
    const uint32_t bitpos = h % bits_;
    if ((array_[bitpos / 8] & (1 << (bitpos % 8))) == 0)
      return false;
    h += delta;
  }
  return true;
}

void BloomFilter::Add(const Slice& key) {
  uint32_t h = Hash(key.data(), key.size(), SEED);
  const uint32_t delta_ = (h >> 17) | (h << 15);
  for (size_t j = 0; j < k_; ++ j) {
    const uint32_t bitpos = h % bits_;
    array_[bitpos / 8] |= (1 << (bitpos % 8));
    h += delta;
  }  
}

std::string BloomFilter::ToString() {
  std::string data;
  data = Util::LongToString(bits_);
  data = data + std::string(array_);
  return data;
}

}