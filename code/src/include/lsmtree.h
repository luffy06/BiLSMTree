#ifndef BILSMTREE_LSMTREE_H
#define BILSMTREE_LSMTREE_H

#include <cmath>

#include <vector>
#include <queue>
#include <algorithm>

#include "visitfrequency.h"
#include "tableiterator.h"

namespace bilsmtree {

class LSMTree {
public:
  LSMTree(FileSystem* filesystem, LSMTreeResult* lsmtreeresult);
  
  ~LSMTree();

  bool Get(const Slice key, Slice& value);

  void AddTableToL0(const std::vector<KV>& kvs);

  void Splay(const double read_ratio);

  bool ReadyForRollback() {
    return rollback_datas_ != NULL;
  }

  std::vector<KV> GetRollbackData() {
    std::vector<KV> res_;
    while (rollback_datas_ != NULL && rollback_datas_->HasNext())
      res_.push_back(rollback_datas_->Next());

    delete rollback_datas_;
    rollback_datas_ = NULL;
    return res_;
  }

private:
  std::vector<Meta> file_[Config::LSMTreeConfig::MAX_LEVEL];
  std::vector<Meta> buffer_[Config::LSMTreeConfig::MAX_LEVEL];
  std::vector<size_t> cur_size_;
  std::vector<size_t> plus_size_;
  std::vector<size_t> base_size_;
  std::vector<size_t> buf_size_;
  VisitFrequency *recent_files_;
  std::vector<size_t> frequency_;
  TableIterator* rollback_datas_;
  size_t total_sequence_number_;
  FileSystem *filesystem_;
  LSMTreeResult *lsmtreeresult_;
  size_t rollback_;
  double ALPHA;
  double read_ratio_;
  const std::vector<std::string> variable_tree_algos = {"BiLSMTree", "BiLSMTree-KV", "BiLSMTree-Ext"};
  const std::vector<std::string> rollback_algos = {"BiLSMTree", "BiLSMTree-KV", "BiLSMTree-Direct", "BiLSMTree-Ext", "BiLSMTree-Mem"};
  const std::vector<std::string> rollback_with_cuckoo_algos = {"BiLSMTree-Ext"};
  const std::vector<std::string> rollback_direct_algos = {"BiLSMTree", "BiLSMTree-KV"};
  const std::vector<std::string> rollback_top_algos = {"BiLSMTree-Direct"};
  const std::vector<std::string> rollback_mem_algos = {"BiLSMTree-Mem"};
  const std::vector<std::string> bloom_algos = {"Wisckey", "LevelDB", "BiLSMTree", "BiLSMTree-KV", "BiLSMTree-Direct", "BiLSMTree-Mem"};
  const std::vector<std::string> cuckoo_algos = {"BiLSMTree-Ext", "Cuckoo"};

  size_t GetSequenceNumber();

  std::string GetFilename(size_t sequence_number_);

  int BinarySearch(size_t level, const Slice key);

  void UpdateFrequency(size_t sequence_number);

  std::vector<size_t> GetCheckFiles(std::string algo, size_t level, const Slice key);

  void GetOverlaps(std::vector<Meta>& src, std::vector<Meta>& des);

  bool GetValueFromFile(const Meta meta, const Slice key, Slice& value);

  size_t GetTargetLevel(const size_t now_level, const Meta meta);

  TableIterator* RemoveExpiredData(const size_t now_level, const size_t to_level, const Meta meta);

  void RollBackWithCuckoo(const size_t now_level, const Meta meta);

  void RollBack(const size_t now_level, const Meta meta);

  void RollBackMem(const size_t now_level, const Meta meta);

  std::vector<Table*> MergeTables(const std::vector<TableIterator*>& tables);

  void CompactList(size_t level);

  void MajorCompaction(size_t level);

  bool CheckFileList(size_t level);

  void ShowMetas(size_t level, bool show_buffer);
};
}

#endif