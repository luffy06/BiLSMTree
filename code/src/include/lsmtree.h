namespace bilsmtree {
class LSMTreeConfig {
public:
  LSMTreeConfig();
  ~LSMTreeConfig();
  
  const static int LEVEL = 7;
  const static
};

class LSMTree {
public:
  LSMTree();
  ~LSMTree();
  
  void Put(const KV& kv);

  Slice Get(const Slice& key);

  void AddTable(const Table* table);

  void Compact(int level);

  void Rollback(int file_number);

  int GetSequenceNumber();
private:
  std::vector<Meta> file_[LSMTreeConfig::LEVEL];
  std::queue<int> recent_files_;

  int sequnce_number_;
};
}