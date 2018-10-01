#include "filesystem.h"

class FatFileSystem : public FileSystem {
public:
  FatFileSystem();
  ~FatFileSystem();

  
private:
  const int MAX_BLOCK_NUMS = (1 << 16) * 4;
  const int CLUSTER_NUMS = 64;


};