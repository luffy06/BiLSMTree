namespace bilsmtree {

class TableConfig {
public:
  TableConfig();
  ~TableConfig();

  const static size_t BLOCKSIZE = 4 * 1024; // 4KB
  const static size_t FILTERSIZE = 4 * 1024;
  const static size_t FOOTERSIZE = 2 * sizeof(size_t);
  const static size_t TABLESIZE = 2 * 1024 * 1024; // 2MB
  const static std::string TABLEPATH = "../logs/leveldb/";
  const static std::string TABLENAME = "sstable";
};

class Block {
public:
  Block(const char* data, const size_t size) : data_(data), size_(size) { }
  
  ~Block();

  char* data() { return data_; }

  size_t size() { return size_; }
private:
  const char* data_;
  size_t size_;
};

class Table {
public:
  Table();

  Table(const std::vector<KV>& kvs);
  
  ~Table();

  Meta GetMeta();

  void DumpToFile(const std::string& filename);
private:
  Block** data_blocks_;
  Block* index_block_;
  Filter* filter_;
  size_t data_block_number_;
  Meta meta;
};


}