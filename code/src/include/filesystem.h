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
private:
  const std::string FILE_META_PATH = "";
  const int MAX_FILE_OPEN = 1000;

  struct FileStatus {
    int file_number;
    int pos;
    int lba;
    int mode;
  };

  std::vector<FileStatus> file_buffer;
};