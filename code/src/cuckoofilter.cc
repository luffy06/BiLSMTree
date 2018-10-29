namespace bilsmtree {

Bucket::Bucket(const size_t max_bits) {
  max_bits_ = max_bits;
  data.resize(max_bits_);
}

CuckooFilter::CuckooFilter(const size_t capacity, const std::vector<Slice>& keys) {
  assert(capacity > 0);
  capacity_ = capacity;
  array_ = new Bucket[capacity_];
  for (size_t i = 0; i < capacity_; ++ i)
    array_[i] = new Bucket(CuckooFilterConfig::MAXBUCKETSIZE);

  for (size_t i = 0; i < keys.size(); ++ i)
    Add(keys[i]);
}

void CuckooFilter::Add(const Slice& key) {
  Slice f = GetFingerPrint(key);
  uint32_t p1 = Hash(key.data(), key.size(), seed_) % capacity_;
  uint32_t p2 = (p1 ^ Hash(f.data(), f.size(), seed_)) % capacity_;
  if (array_[p1].size() )
}

}