namespace bilsmtree {
class LSMTreeConfig {
public:
  LSMTreeConfig();
  ~LSMTreeConfig();
  
  const static size_t LEVEL = 7;
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
  VisitFrequency recent_files_;

  size_t sequence_number_;

  void RollBack(size_t file_number);

  void MajorCompact(size_t level);
};
}