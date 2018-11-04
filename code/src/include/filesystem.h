namespace bilsmtree {

class FileSystemConfig {
public:
  FileSystemConfig();

  ~FileSystemConfig();

  const static size_t READ_OPTION = 1;
  const static size_t WRITE_OPTION = 1 << 1;
  const static size_t APPEND_OPTION = 1 << 2;
};

struct FCB {
  std::string filename_;
  size_t block_start_;
  size_t filesize_; // the number of blocks

  FCB(std::string a, size_t b, size_t c) {
    filename_ = a;
    block_start_ = b;
    filesize_ = c;
  }
};

struct FileStatus {
  size_t file_number_;
  size_t lba_;
  size_t offset_
  size_t mode_;
  
  FileStatus() {
    file_number_ = 0;
    lba_ = 0;
    offset_ = 0;
    mode_ = 0;
  }
};

class FileSystem {
public:
  FileSystem();
  ~FileSystem();

  static size_t Open(const std::string& filename, const size_t mode);

  static void Seek(const size_t file_number, const size_t offset);

  static std::string Read(const size_t file_number, const size_t read_size);

  static void Write(const size_t file_number, const char* data, const size_t write_size);

  static void Close(const size_t file_number);  
private:
  const size_t BLOCK_SIZE = 16;
  const size_t MAX_BLOCK_NUMS = (1 << 16) * 4;
  const size_t CLUSTER_NUMS = 64;
  const std::string FILE_META_PATH = "../logs/filesystem.txt";
  const size_t MAX_FILE_OPEN = 1000;  

  std::vector<FileStatus> file_buffer_;
  std::vector<FCB> fcbs_;
  size_t *fat_;
  size_t open_number_;
  Flash *flash_;
  std::queue<size_t> free_blocks_;

  size_t AssignFreeBlocks();

  void FreeBlock(const size_t lba);

  int BinarySearchInBuffer(const size_t file_number);
};
}