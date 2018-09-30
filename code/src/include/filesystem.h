#include <vector>
#include <string>

class FileSystem {
public:
  FileSystem();
  ~FileSystem();
private:
  struct LBA {
    std::string filename;

  };

  std::vector<LBA> lbas;
};