#include <vector>
#include <string>

class FileSystem {
public:
  FileSystem();
  ~FileSystem();

  int open(const std::string &filename, const int &mode);

  void read(const int &file_number, std::string &data);

  void write(const int &file_number, const std::string &data);

  void close(const int &file_number);

  void seek(const int &file_number, const int &offset);

  int tell();
private:
  const int MAX_FILE_OPEN = 1000;
  const int LOGICAL_PAGE_SIZE = 16 * 1024;

  struct LBA {
    std::string filename;
    std::vector<std::pair<int, int>> block_nums;
    std::vector<int> page_nums;
  };

  struct FileStatus {
    int file_number;
    int pos;
    int lba;
    int mode;
  };

  std::vector<LBA> lbas;

  std::vector<FileStatus> file_buffer;
};