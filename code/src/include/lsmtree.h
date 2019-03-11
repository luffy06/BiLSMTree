#ifndef BILSMTREE_LSMTREE_H
#define BILSMTREE_LSMTREE_H

#include <cmath>

#include <vector>
#include <queue>
#include <algorithm>

#include "visitfrequency.h"
#include "tableiterator.h"
#include "datamanager.h"
#include "indextableiterator.h"
#include "blockiterator.h"

namespace bilsmtree {

class LSMTree {
public:
  LSMTree(FileSystem* filesystem, LSMTreeResult* lsmtreeresult);
  
  ~LSMTree();

  bool Get(const Slice key, Slice& value);

  void AddTableToL0(const std::vector<KV>& kvs);

private:
  std::vector<Meta> file_[Config::LSMTreeConfig::MAX_LEVEL];
  std::vector<Meta> buffer_[Config::LSMTreeConfig::MAX_LEVEL];
  std::vector<size_t> max_size_;
  std::vector<size_t> min_size_;
  std::vector<size_t> buf_size_;
  VisitFrequency *recent_files_;
  DataManager *datamanager_;
  std::vector<size_t> frequency_;
  size_t total_sequence_number_;
  FileSystem *filesystem_;
  LSMTreeResult *lsmtreeresult_;
  size_t rollback_;
  double ALPHA;
  const std::vector<std::string> variable_tree_algos = {"BiLSMTree"};
  const std::vector<std::string> rollback_base_algos = {"BiLSMTree-Direct", "BiLSMTree"};
  const std::vector<std::string> rollback_buffer_algos = {"BiLSMTree"};
  const std::vector<std::string> rollback_direct_algos = {"BiLSMTree-Direct"};
  const std::vector<std::string> bloom_algos = {"Wisckey", "LevelDB", "BiLSMTree-Direct"};
  const std::vector<std::string> cuckoo_algos = {"BiLSMTree", "Cuckoo"};
  const std::vector<std::string> keep_block_algos = {"LevelDB-Sep"};
  const std::vector<std::string> tiered_algos = {};

  size_t GetSequenceNumber();

  std::string GetFilename(size_t sequence_number_);

  int BinarySearch(size_t level, const Slice key);

  void UpdateFrequency(size_t sequence_number);

  std::vector<size_t> GetCheckFiles(std::string algo, size_t level, const Slice key);

  void GetOverlaps(std::vector<Meta>& src, std::vector<Meta>& des);

  bool GetValueFromFile(const Meta meta, const Slice key, Slice& value);

  size_t GetTargetLevel(const size_t now_level, const Meta meta);

  void RollBack(const size_t now_level, const Meta meta);

  void RollBackBase(const size_t now_level, const Meta meta);

  std::vector<Table*> MergeTables(const std::vector<TableIterator*>& tables);

  std::vector<Table*> MergeIndexTables(const std::vector<IndexTableIterator*>& tables);

  void MergeBlocks(const std::vector<BlockIterator*>& bis, std::vector<KV>& kv_buffers);

  void CompactList(size_t level);

  void MajorCompaction(size_t level);

  bool CheckFileList(size_t level);

  void ShowMetas(size_t level, bool show_buffer);
};
}

#endif