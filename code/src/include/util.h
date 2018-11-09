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
  
  static std::string IntToString(uint value);

  static uint StringToInt(const std::string value);

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
    static const uint BITS_PER_KEY = 10;
    static const uint CUCKOOFILTER_SIZE = 10000;  
    static const size_t SEED = 0xbc91f1d34;
  };

  struct FlashConfig {
    static constexpr const double READ_WRITE_RATE = 4.;
    static constexpr const char* BASE_PATH = "../logs/";
    static constexpr const char* LOG_PATH = "../logs/flashlog.txt";
    static const uint BLOCK_NUMS = 256;
    static const uint PAGE_NUMS = 8;
    static const uint PAGE_SIZE = 16 * 1024; // 16KB
    static const uint LBA_NUMS = BLOCK_NUMS * PAGE_NUMS;
    static const uint LOG_LENGTH = 1000;
    static const uint BLOCK_THRESOLD = static_cast<uint>(BLOCK_NUMS * 0.8);
  };

  struct LogManagerConfig {
    static constexpr const char* LOG_PATH = "../logs/VLOG";
    static const uint GARBARGE_THRESOLD = 20;
  };

  struct LRU2QConfig {
    static const uint M1 = 1000;    // size of lru
    static const uint M2 = 1000;    // size of fifo
  };

  struct LSMTreeConfig {
    static const uint LEVEL = 7;
    static const uint L0SIZE = 4;
    static constexpr const double ALPHA = 0.2;
    static const uint LISTSIZE = 4;
  };

  struct SkipListConfig {
    static constexpr const double PROB = 0.5;
    static const uint MAXLEVEL = 4096;
  };

  struct ImmutableMemTableConfig {
    static const uint MAXSIZE = 4096;
  };

  struct TableConfig {
    static const uint BLOCKSIZE = 4 * 1024; // 4KB
    static const uint FILTERSIZE = 4 * 1024;
    static const uint FOOTERSIZE = 2 * sizeof(uint);
    static const uint TABLESIZE = 2 * 1024 * 1024; // 2MB
    static constexpr const char* TABLEPATH = "../logs/leveldb/";
    static constexpr const char* TABLENAME = "sstable";
  };

  struct VisitFrequencyConfig {
    static const uint MAXQUEUESIZE = 100000;
    static constexpr const char* FREQUENCYPATH = "../logs/frequency.log";
  };

  struct CacheServerConfig {
    static const uint MAXSIZE = 10; // max size of immutable memtables
  };

  struct CuckooFilterConfig {
    static const uint MAXBUCKETSIZE = 4;
  };

  struct FileSystemConfig {
    static const uint READ_OPTION = 1;
    static const uint WRITE_OPTION = 1 << 1;
    static const uint APPEND_OPTION = 1 << 2;
    static const uint MAX_FILE_OPEN = 1000;
    static const uint BLOCK_SIZE = 16;
    static const uint MAX_BLOCK_NUMS = (1 << 16) * 4;
    static const uint CLUSTER_NUMS = 64;
    static constexpr const char* FILE_META_PATH = "../logs/filesystem.txt";
  };
};

}

#endif