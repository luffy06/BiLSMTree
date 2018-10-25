namespace bilsmtree {

class TableConfig {
public:
  TableConfig();
  ~TableConfig();

  const static size_t BLOCKSIZE = 4 * 1024; // 4KB
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

struct Meta {
  Slice largest_;
  Slice smallest_;
  int file_number_;
  int frequency_;
};

class Table {
public:
  Table();

  Table(const std::vector<KV>& kvs, int sequence_number);
  
  ~Table();

private:
  Block** data_blocks_;
  Block* index_block_;
  Filter* filter_;
  int data_block_number_;
  int file_number_;
};


}