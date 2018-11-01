namespace bilsmtree {

class TableConfig {
public:
  TableConfig();
  ~TableConfig();

  const static size_t BLOCKSIZE = 4 * 1024; // 4KB
  const static size_t FILTERSIZE = ;
  const static size_t FOOTERSIZE = ;
  const static std::string TABLEPATH = "../logs/leveldb/";
};

class Block {
public:
  Block(const char* data, const size_t size) : data_(data), size_(size) { }
  ~Block();
private:
  const char* data_;
  size_t size_;
};

class Table {
public:
  Table();

  Table(const std::vector<KV>& kvs, size_t sequence_number);
  
  ~Table();

private:
  Block** data_blocks_;
  Block* index_block_;
  Filter* filter_;
  size_t data_block_number_;
  size_t sequence_number_;
};


}