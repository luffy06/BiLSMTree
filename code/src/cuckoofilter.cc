namespace bilsmtree {

Bucket::Bucket() {
  data_ = new Slice[CuckooFilterConfig::MAXBUCKETSIZE];
  for (size_t i = 0; i < CuckooFilterConfig::MAXBUCKETSIZE; ++ i)
    data_[i] = NULL;
  size_ = 0;
}

bool Bucket::Insert(const Slice& fp) {
  if (size_ < CuckooFilterConfig::MAXBUCKETSIZE) {
    data_[size_] = fp;
    return true;
  }
  return false;
}

Slice Bucket::Kick(const Slice& fp) {
  size_t i = rand() % size_;
  Slice res = data_[i];
  data_[i] = fp;
  return res;
}

bool Bucket::Delete(const Slice& fp) {
  bool found = false;
  for (size_t i = 0; i < size_; ++ i) {
    if (found)
      data_[i - 1] = data_[i];
    if (data_[i] == fp)
      found = true;
  }
  if (found)
    size_ -= 1;
  return found;
}

bool Bucket::Find(const Slice& fp) {
  for (size_t i = 0; i < size_; ++ i)
    if (data_[i] == fp)
      return true;
  return false;
}

void Bucket::Diff(const uint32_t& pos, const CuckooFilter* cuckoofilter) {
  std::vector<size_t> deleted_indexs;
  for (size_t i = 0; i < size_; ++ i) {
    if (cuckoofilter.FindFingerPrint(data_[i], pos))
      deleted_indexs.push_back(i);
  }
  for (size_t i = 0, j = 0, k = 0; k < deleted_indexs.size();) {
    while (i < size_ && k < deleted_indexs.size() && i == deleted_indexs[k]) {
      ++ i;
      ++ k;
    }
    data_[j] = data_[i];
    ++ j;
    ++ i;
  }
  size_ = size_ - deleted_indexs.size();
}

CuckooFilter::CuckooFilter(const size_t capacity, const std::vector<Slice>& keys) {
  assert(capacity > 0);
  capacity_ = capacity;
  array_ = new Bucket[capacity_];
  size_ = 0;
  for (size_t i = 0; i < capacity_; ++ i)
    array_[i] = new Bucket();

  for (size_t i = 0; i < keys.size(); ++ i)
    Add(keys[i]);
}

CuckooFilter::CuckooFilter(std::string data) {
  std::istringstream is(data);
  char temp[100];
  is.read(temp, sizeof(size_t));
  capacity_ = Util::StringToLong(std::string(temp));
  is.read(temp, sizeof(size_t));
  size_ = Util::StringToLong(std::string(temp));
  array_ = new Bucket[capacity_];
  for (size_t i = 0; i < capacity_; ++ i)
    array_[i] = new Bucket();
  for (size_t i = 0; i < size_; ++ i) {
    is.read(temp, sizeof(size_t));
    array_[i] -> size_ = Util::StringToLong(std::string(temp));
    array_[i] -> data_ = new Slice[CuckooFilterConfig::MAXBUCKETSIZE];
    for (size_t j = 0; j < array_[i] -> size_; ++ j)
      array_[i] -> data_[j] = NULL;
    for (size_t j = 0; j < array_[i] -> size_; ++ j) {
      is.read(temp, sizeof(uint32_t));
      array_[i] -> data_[j] = new Slice(temp);
    }
  }
}

CuckooFilter::~CuckooFilter() {
  delete[] array_;
}

bool CuckooFilter::Delete(const Slice& key) {
  Info info = GetInfo(key);
  return array_[info.pos1].Delete(info.fp_) || array_[info.pos2].Delete(info.fp_);  
}

bool CuckooFilter::KeyMatch(const Slice& key) {
  Info info = GetInfo(key);
  return array_[info.pos1].Find(info.fp_) || array_[info.pos2].Find(info.fp_);  
}

void CuckooFilter::Diff(const CuckooFilter* cuckoofilter) {
  for (size_t i = 0; i < capacity_; ++ i)
    array_[i].Diff(i, cuckoofilter);
}

bool CuckooFilter::FindFingerPrint(const Slice& fp, const uint32_t& pos) {
  if (data_[pos].Find(fp))
    return true;
  uint32_t alt_pos = GetAlternatePos(fp, pos);
  return data_[alt_pos].Find(fp);
}

Slice CuckooFilter::GetFingerPrint(const Slice& key) {
  uint32_t f = Hash(key.data(), key.size(), FPSEED) % 255 + 1;
  char str = new char[128];
  sprintf(str, "%d", f);
  return Slice(std::string(str));
}

Info GetInfo(const Slice& key) {
  Slice fp = GetFingerPrint(key);
  uint32_t p1 = Hash(key.data(), key.size(), SEED) % capacity_;
  uint32_t p2 = GetAlternatePos(fp, p1);
  return Info(f, p1, p2);  
}

uint32_t GetAlternatePos(const Slice& fp, const uint32_t& p) {
  uint32_t hash = Hash(fp.data(), fp.size(), SEED);
  return (p ^ hash) % capacity_;
}

void CuckooFilter::InsertAndKick(const Slice& fp, const uint32_t pos) {
  Slice fp_ = fp;
  for (size_t i = 0; i < MAXKICK; ++ i) {
    fp_ = array_[pos].Kick(fp_);
    pos = GetAlternatePos(fp_, pos);
    if (array_[pos].Insert(fp_))
      return ;
  }
  Util::Assert('SPACE IS NOT ENOUGH', false);
}

void CuckooFilter::Add(const Slice& key) {
  Info info = GetInfo(key);
  if (array_[info.pos1].Insert(info.fp_) || array_[info.pos2].Insert(info.fp_))
    return ;
  InsertAndKick(info.fp_, rand() % 2 == ? info.pos1 : info.pos2);
  size_ = size_ + 1;
}

std::string CuckooFilter::ToString() {
  std::string data;
  data = Util::LongToString(capacity_) + Util::LongToString(size_);
  for (size_t i = 0; i < size_; ++ i) {
    data = data + Util::LongToString(array_[i] -> size_);
    for (size_t j = 0; j < array_[i] -> size_; ++ j) 
      data = data + array_[i] -> data_[j] -> ToString();
  }
  return data;
}

}