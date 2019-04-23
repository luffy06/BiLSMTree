#ifndef BILSMTREE_UTIL_H
#define BILSMTREE_UTIL_H

#include <cstring>
#include <cassert>

#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
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

  static size_t GetMemTableNumb();

  static size_t GetMemTableSize();

  static size_t GetLRU2QSize();

  static size_t GetSSTableSize();

  static size_t GetBlockSize();

  static bool CheckAlgorithm(const std::string &algo, const std::vector<std::string> &algos);

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

  static constexpr const char* ALGO_PATH = "config.in";
  static const char DATA_SEG = '\t';
  static const bool FILESYSTEM_LOG = false;
  static const bool FLASH_LOG = false;
  static const bool TRACE_LOG = false;
  static const bool TRACE_READ_LOG = false;
  static const bool WRITE_OUTPUT = false;
  static const size_t MAX_SCAN_NUMB = 100;
  static const size_t BUFFER_SIZE = 2000000;

  struct FlashConfig {
    static const size_t READ_LATENCY = 50;            // 50us
    static const size_t WRITE_LATENCY = 200;          // 200us
    static const size_t ERASE_LATENCY = 2000;         // 2ms
    static constexpr const double READ_WRITE_RATE = (1.0 * WRITE_LATENCY) / READ_LATENCY;
    static constexpr const char* BASE_PATH = "logs/";
    static const size_t BLOCK_NUMS = 8192;
    static const size_t PAGE_NUMS = 128;
    static const size_t PAGE_SIZE = 8 * 1024; // 8KB
    static const size_t LBA_NUMS = BLOCK_NUMS * PAGE_NUMS;
    static const size_t LOG_LENGTH = 5000;
    static const size_t BLOCK_COLLECTION_TRIGGER = static_cast<size_t>(LBA_NUMS * 0.2);
    static const size_t BLOCK_COLLECTION_THRESOLD = static_cast<size_t>(LBA_NUMS * 0.8);
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
    static const size_t IMM_NUMB = 3;
    static const size_t MEMORY_SIZE = 8 * 1024 * 1024;
    static const size_t LRU2Q_RATE = 3;
    static const size_t LRU_RATE = 1;
  };

  struct FilterConfig {
    static const size_t BITS_PER_KEY = 12;
    static const size_t MAXBUCKETSIZE = 4;
    static const size_t PADDING = 1000000007;
    static const size_t SEED = 0xbc91f1d34;
  };

  struct LogManagerConfig {
    static constexpr const char* LOG_PATH = "../logs/VLOG";
    static const size_t GARBARGE_THRESOLD = 20;
  };

  struct LSMTreeConfig {
    static const size_t MAX_LEVEL = 7;
    static const size_t L0SIZE = 4;
    static const size_t LIBASE = 10;
    static const size_t LISTSIZE = 50;
  };

  struct TableConfig {
    static const size_t LEVELDB_TABLE_SIZE = 2 * 1024 * 1024;
    static const size_t LEVELDB_BLOCK_SIZE = 512;
    static const size_t WISCKEY_TABLE_SIZE = 32 * 1024;
    static const size_t WISCKEY_BLOCK_SIZE = 16;
    static const size_t BILSMTREE_TABLE_SIZE = 8 * 1024;
    static const size_t BILSMTREE_BLOCK_SIZE = 16;
    static const size_t BILSMTREE_DIRECT_TABLE_SIZE = 8 * 1024;
    static const size_t BILSMTREE_DIRECT_BLOCK_SIZE = 16;
  };

  struct VisitFrequencyConfig {
    static const size_t MAXQUEUESIZE = 100000;
    static constexpr const char* FREQUENCYPATH = "../logs/frequency.log";
  };

};

class FlashResult {
public:
  FlashResult() {
    latency_ = 0;
    erase_times_ = 0;
    read_times_ = 0;
    write_times_ = 0;
    record_ = false;
  }

  ~FlashResult() {

  }

  void Read() {
    if (record_) {
      latency_ = latency_ + Config::FlashConfig::READ_LATENCY;
      read_times_ = read_times_ + 1;
    }
  }

  void Write() {
    if (record_) {
      latency_ = latency_ + Config::FlashConfig::WRITE_LATENCY;
      write_times_ = write_times_ + 1;
    }
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

  size_t GetReadTimes() {
    return read_times_;
  }

  size_t GetWriteTimes() {
    return write_times_;
  }

  size_t GetEraseTimes() {
    return erase_times_;
  }

  void StartRecord() {
    record_ = true;
  }

  void ShowResult() {
    std::cout << "LATENCY:" << GetLatency() << std::endl;
    std::cout << "READ_TIMES:" << GetReadTimes() << std::endl;
    std::cout << "WRITE_TIMES:" << GetWriteTimes() << std::endl;
    std::cout << "ERASE_TIMES:" << GetEraseTimes() << std::endl;
  }

private:
  bool record_;
  size_t latency_;
  size_t read_times_;
  size_t write_times_;
  size_t erase_times_;
};

class LSMTreeResult {
public:
  LSMTreeResult() {
    read_files_ = 0;
    read_size_ = 0;
    real_read_size_ = 0;
    read_in_flash_ = 0;
    read_in_memory_ = 0;
    write_files_ = 0;
    write_size_ = 0;
    real_write_size_ = 0;
    minor_compaction_times_ = 0;
    minor_compaction_size_ = 0;
    major_compaction_times_ = 0;
    major_compaction_size_ = 0;
    check_times_.clear();
    merge_size_.clear();
    max_merge_size_ = 0;
    rollback_ = 0;
    record_ = false;
  }

  ~LSMTreeResult() {
  }

  void RealRead(size_t size) {
    if (record_) {
      real_read_size_ = real_read_size_ + size;
    }
  }

  void ReadInFlash() {
    if (record_) {
      read_in_flash_ = read_in_flash_ + 1;
    }
  }

  void ReadInMemory() {
    if (record_) {
      read_in_memory_ = read_in_memory_ + 1;
    }
  }

  void Read(size_t size, const std::string type) {
    if (record_) {
      read_files_ = read_files_ + 1;
      read_size_ = read_size_ + size;
      if (read_map_.find(type) != read_map_.end())
        read_map_[type] = read_map_[type] + size;
      else
        read_map_[type] = size;
    }
  }

  void RealWrite(size_t size) {
    if (record_) {
      real_write_size_ = real_write_size_ + size;
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

  void MajorCompaction(size_t file_size, size_t merge_size) {
    if (record_) {
      major_compaction_times_ = major_compaction_times_ + 1;
      major_compaction_size_ = major_compaction_size_ + file_size;
      merge_size_.push_back(merge_size);
      max_merge_size_ = std::max(max_merge_size_, merge_size);
    }
  }

  void Check(size_t times) {
    if (record_) {
      check_times_.push_back(times);
    }
  }

  void RollBack() {
    if (record_) {
      rollback_ = rollback_ + 1;
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

  double GetAverageReadSize() {
    if (read_files_ == 0)
      return 0;
    return read_size_ * 1.0 / read_files_;
  }

  size_t GetRealReadSize() {
    return real_read_size_;
  }

  size_t GetReadInFlash() {
    return read_in_flash_;
  }

  size_t GetReadInMemory() {
    return read_in_memory_;
  }

  size_t GetWriteSize() {
    return write_size_;
  }

  double GetAverageWriteSize() {
    if (write_files_ == 0)
      return 0;
    return write_size_ * 1.0 / write_files_;
  }

  size_t GetRealWriteSize() {
    return real_write_size_;
  }

  size_t GetMinorCompactionSize() {
    return minor_compaction_size_;
  }

  double GetAverageMinorCompactionSize() {
    if (minor_compaction_times_ == 0)
      return 0;
    return minor_compaction_size_ * 1.0 / minor_compaction_times_;
  }

  size_t GetMajorCompactionSize() {
    return major_compaction_size_;
  }

  double GetAverageMajorCompactionSize() {
    if (major_compaction_times_ == 0)
      return 0;
    return major_compaction_size_ * 1.0 / major_compaction_times_;
  }

  double GetCheckTimesAvg() {
    double sum = 0;
    for (size_t i = 0; i < check_times_.size(); ++ i)
      sum = sum + check_times_[i];
    if (check_times_.size() == 0)
      return 0;
    return (sum / check_times_.size());
  }

  double GetMergeSizeAvg() {
    double sum = 0;
    for (size_t i = 0; i < merge_size_.size(); ++ i)
      sum = sum + merge_size_[i];
    if (merge_size_.size() == 0)
      return 0;
    return (sum / merge_size_.size());
  }

  size_t GetMaxMergeSize() {
    return max_merge_size_;
  }

  size_t GetRollBack() {
    return rollback_;
  }

  void StartRecord() {
    record_ = true;
  }

  void ShowResult() {
    std::cout << "READ_FILES:" << GetReadFiles() << std::endl;
    std::cout << "READ_SIZE:" << GetReadSize() << std::endl;
    std::cout << "REAL_READ_SIZE:" << GetRealReadSize() << std::endl;
    std::cout << "READ_AMPLIFICATION:" << 1.0 * GetReadSize() / GetRealReadSize() << std::endl;
    std::cout << "AVG_READ_SIZE:" << std::setprecision(6) << GetAverageReadSize() << std::endl;
    std::cout << "READ_IN_FLASH:" << GetReadInFlash() << std::endl;
    std::cout << "READ_IN_MEMORY:" << GetReadInMemory() << std::endl;
    std::cout << "WRITE_FILES:" << GetWriteFiles() << std::endl;
    std::cout << "WRITE_SIZE:" << GetWriteSize() << std::endl;
    std::cout << "REAL_WRITE_SIZE:" << GetRealWriteSize() << std::endl;
    std::cout << "WRITE_AMPLIFICATION:" << 1.0 * GetWriteSize() / GetRealWriteSize() << std::endl;
    std::cout << "AVG_WRITE_SIZE:" << GetAverageWriteSize() << std::endl;
    std::cout << "MINOR_COMPACTION:" << GetMinorCompactionTimes() << std::endl;
    std::cout << "MINOR_COMPACTION_SIZE:" << GetMinorCompactionSize() << std::endl;
    std::cout << "AVG_MINOR_COMPACTION_SIZE:" << GetAverageMinorCompactionSize() << std::endl;
    std::cout << "MAJOR_COMPACTION:" << GetMajorCompactionTimes() << std::endl;
    std::cout << "MAJOR_COMPACTION_SIZE:" << GetMajorCompactionSize() << std::endl;
    std::cout << "AVG_MAJOR_COMPACTION_SIZE:" << GetAverageMajorCompactionSize() << std::endl;
    std::cout << "AVERAGE_CHECK_TIMES:" << GetCheckTimesAvg() << std::endl;
    std::cout << "AVERAGE_MERGE_SIZE:" << GetMergeSizeAvg() << std::endl;
    std::cout << "MAX_MERGE_SIZE:" << GetMaxMergeSize() << std::endl;
    std::cout << "ROLLBACK:" << GetRollBack() << std::endl;
    for (std::map<std::string, size_t>::iterator it = read_map_.begin(); it != read_map_.end(); ++ it)
      std::cout << "TYPE_" << it->first << ":" << it->second << std::endl;
  }

private:
  bool record_;
  size_t write_files_;
  size_t write_size_;
  size_t real_write_size_;
  size_t read_files_;
  size_t read_size_;
  size_t real_read_size_;
  size_t read_in_flash_;
  size_t read_in_memory_;
  size_t minor_compaction_times_;
  size_t minor_compaction_size_;
  size_t major_compaction_times_;
  size_t major_compaction_size_;
  size_t max_merge_size_;
  std::vector<size_t> check_times_;
  std::vector<size_t> merge_size_;
  std::map<std::string, size_t> read_map_;
  size_t rollback_;
};

class MemoryResult {
public:
  MemoryResult() {
    hit_ = 0;
    total_ = 0;
    record_ = false;
  }

  ~MemoryResult() {

  }

  void Hit(size_t hit) {
    if (record_) {
      hit_ = hit_ + hit;
      total_ = total_ + 1;
    }
  }

  size_t GetHit() {
    return hit_;
  }

  size_t GetTotalQuery() {
    return total_;
  }

  double GetHitRate() {
    if (total_ == 0)
      return 0;
    return (1. * hit_) / total_;
  }

  void StartRecord() {
    record_ = true;
  }

  void ShowResult() {
    std::cout << "HIT:" << GetHit() << std::endl;
    std::cout << "TOTAL_QUERY:" << GetTotalQuery() << std::endl;
    std::cout << "HIT_RATE:" << GetHitRate() << std::endl;
  }
private:
  bool record_;
  size_t hit_;
  size_t total_;
};

class Result {
public:
  Result() {
    flashresult_ = new FlashResult();
    lsmtreeresult_ = new LSMTreeResult();
    memoryresult_ = new MemoryResult();
  }

  ~Result() {
    delete flashresult_;
    delete lsmtreeresult_;
    delete memoryresult_;
  }

  void StartRecord() {
    lsmtreeresult_->StartRecord();
    flashresult_->StartRecord();
    memoryresult_->StartRecord();
  }

  void ShowResult() { 
    flashresult_->ShowResult();
    lsmtreeresult_->ShowResult();
    memoryresult_->ShowResult();
  }

  LSMTreeResult *lsmtreeresult_;
  FlashResult *flashresult_;
  MemoryResult *memoryresult_;
};

}

#endif