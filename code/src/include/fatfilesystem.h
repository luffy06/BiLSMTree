#include "filesystem.h"

class FatFileSystem : public FileSystem {
public:
  virtual FatFileSystem();
  virtual ~FatFileSystem();

  virtual int open(const std::string &filename, const int &mode) ;

  virtual void read(const int &file_number, std::string &data);

  virtual void write(const int &file_number, const std::string &data);

  virtual void close(const int &file_number);

  virtual void seek(const int &file_number, const int &offset);

  virtual int tell();
  
private:
  const int MAX_BLOCK_NUMS = (1 << 16) * 4;
  const int CLUSTER_NUMS = 64;

  int *fat;
};