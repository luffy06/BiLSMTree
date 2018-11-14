#ifndef BILSMTREE_UTIL_H
#define BILSMTREE_UTIL_H

#include <cstring>
#include <cassert>

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

namespace bilsmtree {

class Util {
public:
  Util();
  ~Util();
  
  static std::string IntToString(size_t value);

  static size_t StringToInt(const std::string value);

  static bool ExistFile(const std::string filename);

  static std::string GetAlgorithm();
};

class Config {
public:
  Config() { }

  ~Config() { }

  struct FlashConfig {
    static const size_t READ_LATENCY = 50;            // 50us
    static const size_t WRITE_LATENCY = 200;          // 200us
    static const size_t ERASE_LATENCY = 2000;         // 2ms
    static constexpr const double READ_WRITE_RATE = (1.0 * WRITE_LATENCY) / READ_LATENCY;
    static constexpr const char* BASE_PATH = "../logs/";
    static constexpr const char* LOG_PATH = "../logs/flashlog.txt";
    static const size_t BLOCK_NUMS = 128;
    static const size_t PAGE_NUMS = 8;
    static const size_t PAGE_SIZE = 1024; // 16KB
    static const size_t LBA_NUMS = BLOCK_NUMS * PAGE_NUMS;
    static const size_t LOG_LENGTH = 5000;
    static const size_t BLOCK_COLLECTION_TRIGGER = static_cast<size_t>(BLOCK_NUMS * 0.2);
    static const size_t BLOCK_COLLECTION_THRESOLD = static_cast<size_t>(BLOCK_NUMS * 0.4);
  };

  struct FileSystemConfig {
    static const size_t READ_OPTION = 1;
    static const size_t WRITE_OPTION = 1 << 1;
    static const size_t APPEND_OPTION = 1 << 2;
    static const size_t MAX_FILE_OPEN = 1000;
    static const size_t BLOCK_SIZE = 1024;
    static const size_t MAX_BLOCK_NUMS = (1 << 16) * 4;
  };

  struct SkipListConfig {
    static constexpr const double PROB = 0.5;
    static const size_t MAXLEVEL = 4096;
  };

  struct ImmutableMemTableConfig {
    static const size_t MAXSIZE = 10;
  };

  struct LRU2QConfig {
    static const size_t M1 = 10;    // size of lru
    static const size_t M2 = 10;    // size of fifo
  };

  struct FilterConfig {
    static const size_t BITS_PER_KEY = 10;
    static const size_t CUCKOOFILTER_SIZE = 10000;  
    static const size_t SEED = 0xbc91f1d34;
  };

  struct LogManagerConfig {
    static constexpr const char* LOG_PATH = "../logs/VLOG";
    static const size_t GARBARGE_THRESOLD = 20;
  };

  struct LSMTreeConfig {
    static const size_t LEVEL = 7;
    static const size_t L0SIZE = 4;
    static constexpr const double ALPHA = 0.2;
    static const size_t LISTSIZE = 4;
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
    static const size_t MAXSIZE = 1; // max size of immutable memtables
  };

  struct CuckooFilterConfig {
    static const size_t MAXBUCKETSIZE = 4;
  };

};

class FlashResult {
public:
  FlashResult() {
    latency_ = 0;
    erase_times_ = 0;
  }

  ~FlashResult() {

  }

  void Read() {
    latency_ = latency_ + Config::FlashConfig::READ_LATENCY;
  }

  void Write() {
    latency_ = latency_ + Config::FlashConfig::WRITE_LATENCY;
  }

  void Erase() {
    latency_ = latency_ + Config::FlashConfig::ERASE_LATENCY;
    erase_times_ = erase_times_ + 1;
  }

  size_t GetLatency() {
    return latency_;
  }

  size_t GetEraseTimes() {
    return erase_times_;
  }
private:
  size_t latency_;
  size_t erase_times_;
};

class LSMTreeResult {
public:
  LSMTreeResult() {
    write_files_ = 0;
    read_files_ = 0;
    minor_compaction_times_ = 0;
    major_compaction_times_ = 0;
    check_times_.clear();
  }

  ~LSMTreeResult() {

  }

  void Read() {
    read_files_ = read_files_ + 1;
  }

  void Write() {
    write_files_ = write_files_ + 1;
  }

  void MinorCompaction() {
    minor_compaction_times_ = minor_compaction_times_ + 1;
  }

  void MajorCompaction() {
    major_compaction_times_ = major_compaction_times_ + 1;
  }

  void Check(size_t times) {
    check_times_.push_back(times);
  }

  size_t GetReadFiles() {
    return read_files_;
  }

  size_t GetWriteFiles() {
    return write_files_;
  }

  size_t GetMinorCompactionTimes() {
    return minor_compaction_times_;
  }

  size_t GetMajorCompactionTimes() {
    return major_compaction_times_;
  }

  double GetCheckTimesAvg() {
    double sum = 0;
    for (size_t i = 0; i < check_times_.size(); ++ i)
      sum = sum + check_times_[i];
    return (sum / check_times_.size());
  }
private:
  double write_files_;
  double read_files_;
  size_t minor_compaction_times_;
  size_t major_compaction_times_;
  std::vector<size_t> check_times_;
};

class Result {
public:
  Result() {
    flashresult_ = new FlashResult();
    lsmtreeresult_ = new LSMTreeResult();
  }

  LSMTreeResult *lsmtreeresult_;
  FlashResult *flashresult_;
};

}

#endif