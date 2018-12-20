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

  static size_t GetMemTableSize();

  static size_t GetSSTableSize();

  static void Test(std::string msg) {
    std::string k;
    std::cout << msg << ":";
    std::cin >> k;
  }
};

class Config {
public:
  Config() { }

  ~Config() { }

  static constexpr const char* TRACE_PATH = "trace.in";
  static constexpr const char* ALGO_PATH = "config.in";
  static const char DATA_SEG = '\t';
  static const bool FILESYSTEM_LOG = false;
  static const bool FLASH_LOG = false;
  static const bool TRACE_LOG = false;
  static const bool TRACE_READ_LOG = false;
  static const bool FLASH_TRACE = false;
  static const size_t MAX_SCAN_NUMB = 100;

  struct FlashConfig {
    static const size_t READ_LATENCY = 50;            // 50us
    static const size_t WRITE_LATENCY = 200;          // 200us
    static const size_t ERASE_LATENCY = 2000;         // 2ms
    static constexpr const double READ_WRITE_RATE = (1.0 * WRITE_LATENCY) / READ_LATENCY;
    static constexpr const char* BASE_PATH = "logs/";
    static const size_t BLOCK_NUMS = 4096;
    static const size_t PAGE_NUMS = 32;
    static const size_t PAGE_SIZE = 8 * 1024; // 8KB
    static const size_t LBA_NUMS = BLOCK_NUMS * PAGE_NUMS;
    static const size_t LOG_LENGTH = 5000;
    static const size_t BLOCK_COLLECTION_TRIGGER = static_cast<size_t>(BLOCK_NUMS * 0.2);
    static const size_t BLOCK_COLLECTION_THRESOLD = static_cast<size_t>(BLOCK_NUMS * 0.8);
  };

  struct FileSystemConfig {
    static const size_t READ_OPTION = 1;
    static const size_t WRITE_OPTION = 1 << 1;
    static const size_t APPEND_OPTION = 1 << 2;
    static const size_t MAX_FILE_OPEN = 10;
    static const size_t BLOCK_SIZE = FlashConfig::PAGE_SIZE;
    static const size_t MAX_BLOCK_NUMS = FlashConfig::LBA_NUMS;
  };

  struct SkipListConfig {
    static constexpr const double PROB = 1.;
    static const size_t MAXLEVEL = 4096;
  };

  struct CacheServerConfig {
    static const size_t MAXSIZE = 3;                     // max size of the number of immutable memtables
  };

  struct ImmutableMemTableConfig {
    static const size_t MEM_SIZE = 1024 * 1024;          // 1MB the number of <key, value> stored in immutable memetable
  };

  struct LRU2QConfig {
    static const size_t M1 = ImmutableMemTableConfig::MEM_SIZE;
    static const size_t M2 = ImmutableMemTableConfig::MEM_SIZE;
    static const size_t M1_NUMB = 50;                   // max number of lru
    static const size_t M2_NUMB = 50;                   // max number of fifo    
  };

  struct FilterConfig {
    static const size_t BITS_PER_KEY = 10;
    static const size_t PADDING = 1000000007;
    static const size_t SEED = 0xbc91f1d34;
  };

  struct DataManagerConfig {
    static const size_t MAX_FILE_SIZE = 1024 * 1024;
  };

  struct LogManagerConfig {
    static constexpr const char* LOG_PATH = "../logs/VLOG";
    static const size_t GARBARGE_THRESOLD = 20;
  };

  struct LSMTreeConfig {
    static const size_t MAX_LEVEL = 7;
    static const size_t L0SIZE = 4;
    static const size_t LIBASE = 10;
    static constexpr const double ALPHA = 1.;
    static const size_t LISTSIZE = 10;
    static const size_t TABLE_SIZE = ImmutableMemTableConfig::MEM_SIZE / 512;
  };

  struct TableConfig {
    static const size_t BUFFER_SIZE = 2000000;
    static const size_t BLOCKSIZE = LSMTreeConfig::TABLE_SIZE / 16;
  };

  struct VisitFrequencyConfig {
    static const size_t MAXQUEUESIZE = 100000;
    static constexpr const char* FREQUENCYPATH = "../logs/frequency.log";
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
    record_ = false;
  }

  ~FlashResult() {

  }

  void Read() {
    if (record_)
      latency_ = latency_ + Config::FlashConfig::READ_LATENCY;
  }

  void Write() {
    if (record_)
      latency_ = latency_ + Config::FlashConfig::WRITE_LATENCY;
  }

  void Erase() {
    if (record_) {
      latency_ = latency_ + Config::FlashConfig::ERASE_LATENCY;
      erase_times_ = erase_times_ + 1;
    }
  }

  size_t GetLatency() {
    return latency_;
  }

  size_t GetEraseTimes() {
    return erase_times_;
  }

  void StartRecord() {
    record_ = true;
  }
private:
  bool record_;
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
    record_ = false;
  }

  ~LSMTreeResult() {

  }

  void Read(size_t size) {
    if (record_) {
      read_files_ = read_files_ + 1;
      read_size_ = read_size_ + size;
    }
  }

  void Write(size_t size) {
    if (record_) {
      write_files_ = write_files_ + 1;
      write_size_ = write_size_ + size;
    }
  }

  void MinorCompaction(size_t file_size) {
    if (record_) {
      minor_compaction_times_ = minor_compaction_times_ + 1;
      minor_compaction_size_ = minor_compaction_size_ + file_size;
    }
  }

  void MajorCompaction(size_t file_size) {
    if (record_) {
      major_compaction_times_ = major_compaction_times_ + 1;
      major_compaction_size_ = major_compaction_size_ + file_size;
    }
  }

  void Check(size_t times) {
    if (record_) {
      check_times_.push_back(times);
    }
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

  size_t GetReadSize() {
    return read_size_;
  }

  size_t GetWriteSize() {
    return write_size_;
  }

  size_t GetMinorCompactionSize() {
    return minor_compaction_size_;
  }

  size_t GetMajorCompactionSize() {
    return major_compaction_size_;
  }

  double GetCheckTimesAvg() {
    double sum = 0;
    for (size_t i = 0; i < check_times_.size(); ++ i)
      sum = sum + check_times_[i];
    if (check_times_.size() == 0)
      return 0;
    return (sum / check_times_.size());
  }

  void StartRecord() {
    record_ = true;
  }
private:
  bool record_;
  double write_files_;
  double write_size_;
  double read_files_;
  double read_size_;
  size_t minor_compaction_times_;
  size_t minor_compaction_size_;
  size_t major_compaction_times_;
  size_t major_compaction_size_;
  std::vector<size_t> check_times_;
};

class Result {
public:
  Result() {
    flashresult_ = new FlashResult();
    lsmtreeresult_ = new LSMTreeResult();
  }

  void StartRecord() {
    lsmtreeresult_->StartRecord();
    flashresult_->StartRecord();
  }

  LSMTreeResult *lsmtreeresult_;
  FlashResult *flashresult_;
};

}

#endif