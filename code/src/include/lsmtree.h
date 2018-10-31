namespace bilsmtree {
class LSMTreeConfig {
public:
  LSMTreeConfig();
  ~LSMTreeConfig();
  
  const static size_t LEVEL = 7;
};

struct Meta {
  Slice largest_;
  Slice smallest_;
  size_t sequence_number_;
  size_t file_size_;
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
  VisitFrequency* recent_files_;
  std::vector<size_t> frequency_;
  size_t total_sequence_number_;

  std::string GetFilename(size_t sequence_number_);

  Slice GetFromFile(const Meta& meta, const Slice& key);

  void RollBack(size_t file_number);

  void MajorCompact(size_t level);
};
}