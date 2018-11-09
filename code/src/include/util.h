#ifndef BILSMTREE_UTIL_H
#define BILSMTREE_UTIL_H

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

namespace bilsmtree {

class Util {
public:
  Util();
  ~Util();
  
  static std::string IntToString(size_t value);

  static size_t StringToInt(const std::string value);

  static bool ExistFile(const std::string filename);

  static void Assert(const char* message, bool condition);
};

class Config {
public:
  Config();

  ~Config();
  
  enum Algorithms {
    LevelDB,
    BiLSMTree
  };

  static Algorithms algorithm;

  struct FilterConfig {
    static const size_t BITS_PER_KEY = 10;
    static const size_t CUCKOOFILTER_SIZE = 10000;  
    static const size_t SEED = 0xbc91f1d34;
  };

  struct FlashConfig {
    static constexpr const double READ_WRITE_RATE = 4.;
    static constexpr const char* BASE_PATH = "../logs/";
    static constexpr const char* LOG_PATH = "../logs/flashlog.txt";
    static const size_t BLOCK_NUMS = 256;
    static const size_t PAGE_NUMS = 8;
    static const size_t PAGE_SIZE = 16 * 1024; // 16KB
    static const size_t LBA_NUMS = BLOCK_NUMS * PAGE_NUMS;
    static const size_t LOG_LENGTH = 1000;
    static const size_t BLOCK_THRESOLD = static_cast<size_t>(BLOCK_NUMS * 0.8);
  };

  struct LogManagerConfig {
    static constexpr const char* LOG_PATH = "../logs/VLOG";
    static const size_t GARBARGE_THRESOLD = 20;
  };

  struct LRU2QConfig {
    static const size_t M1 = 1000;    // size of lru
    static const size_t M2 = 1000;    // size of fifo
  };

  struct LSMTreeConfig {
    static const size_t LEVEL = 7;
    static const size_t L0SIZE = 4;
    static constexpr const double ALPHA = 0.2;
    static const size_t LISTSIZE = 4;
  };

  struct SkipListConfig {
    static constexpr const double PROB = 0.5;
    static const size_t MAXLEVEL = 4096;
  };

  struct ImmutableMemTableConfig {
    static const size_t MAXSIZE = 4096;
  };

  struct TableConfig {
    static const size_t BLOCKSIZE = 4 * 1024; // 4KB
    static const size_t FILTERSIZE = 4 * 1024;
    static const size_t FOOTERSIZE = 2 * sizeof(size_t);
    static const size_t TABLESIZE = 2 * 1024 * 1024; // 2MB
    static constexpr const char* TABLEPATH = "../logs/leveldb/";
    static constexpr const char* TABLENAME = "sstable";
  };

  struct VisitFrequencyConfig {
    static const size_t MAXQUEUESIZE = 100000;
    static constexpr const char* FREQUENCYPATH = "../logs/frequency.log";
  };

  struct CacheServerConfig {
    static const size_t MAXSIZE = 10; // max size of immutable memtables
  };

  struct CuckooFilterConfig {
    static const size_t MAXBUCKETSIZE = 4;
  };

  struct FileSystemConfig {
    static const size_t READ_OPTION = 1;
    static const size_t WRITE_OPTION = 1 << 1;
    static const size_t APPEND_OPTION = 1 << 2;
    static const size_t MAX_FILE_OPEN = 1000;
    static const size_t BLOCK_SIZE = 16;
    static const size_t MAX_BLOCK_NUMS = (1 << 16) * 4;
    static const size_t CLUSTER_NUMS = 64;
    static constexpr const char* FILE_META_PATH = "../logs/filesystem.txt";
  };
};

}

#endif