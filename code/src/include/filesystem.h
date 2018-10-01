#include <vector>
#include <string>

class FileSystem {
public:
  virtual FileSystem() = default;
  virtual ~FileSystem() = default;

  virtual int open(const std::string &filename, const int &mode) = 0;

  virtual void read(const int &file_number, std::string &data) = 0;

  virtual void write(const int &file_number, const std::string &data) = 0;

  virtual void close(const int &file_number) = 0;

  virtual void seek(const int &file_number, const int &offset) = 0;

  virtual int tell() = 0;
private:
  const FILE_META_PATH = "";
  const int MAX_FILE_OPEN = 1000;

  struct FileStatus {
    int file_number;
    int pos;
    int lba;
    int mode;
  };

  std::vector<FileStatus> file_buffer;
};