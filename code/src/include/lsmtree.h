namespace bilsmtree {
class LSMTreeConfig {
public:
  LSMTreeConfig();
  ~LSMTreeConfig();
  
  const static size_t LEVEL = 7;
  const static size_t L0SIZE = 4;
  const static double ALPHA = 0.2;
  const static size_t LISTSIZE = 4;
};

struct Meta {
  Slice largest_;
  Slice smallest_;
  size_t sequence_number_;
  size_t level_;
  size_t file_size_;

  bool Intersect(Meta other) {
    if (largest_.compare(other.smallest_) >= 0 && smallest_.compare(other.largest_) <= 0)
      return true;
    return false;
  }
};

class LSMTree {
public:
  LSMTree();
  ~LSMTree();

  Slice Get(const Slice& key);

  void AddTableToL0(const Table* table);

  size_t GetSequenceNumber();
private:
  std::vector<Meta> file_[LSMTreeConfig::LEVEL];
  std::vector<Meta> list_[LSMTreeConfig::LEVEL];
  VisitFrequency* recent_files_;
  size_t min_frequency_number_[LSMTreeConfig::LEVEL];
  std::vector<size_t> frequency_;
  size_t total_sequence_number_;

  std::string GetFilename(size_t sequence_number_);

  Slice GetValueFromFile(const Meta& meta, const Slice& key);

  size_t GetTargetLevel(const size_t now_level, const Meta& meta);

  void RollBack(const size_t now_level, const Meta& meta);

  void CompactList(size_t level);

  void MajorCompact(size_t level);
};
}