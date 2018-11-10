#ifndef BILSMTREE_LSMTREE_H
#define BILSMTREE_LSMTREE_H

#include <cmath>

#include <vector>
#include <queue>
#include <algorithm>

#include "visitfrequency.h"
#include "tableiterator.h"
#include "table.h"

namespace bilsmtree {

class LSMTree {
public:
  LSMTree(FileSystem* filesystem);
  
  ~LSMTree();

  bool Get(const Slice key, Slice& value);

  void AddTableToL0(const std::vector<KV>& kvs);

  size_t GetSequenceNumber();
private:
  std::vector<Meta> file_[Config::LSMTreeConfig::LEVEL];
  std::vector<Meta> list_[Config::LSMTreeConfig::LEVEL];
  VisitFrequency *recent_files_;
  size_t min_frequency_number_[Config::LSMTreeConfig::LEVEL];
  std::vector<size_t> frequency_;
  size_t total_sequence_number_;
  FileSystem* filesystem_;

  std::string GetFilename(size_t sequence_number_);

  bool GetValueFromFile(const Meta meta, const Slice key, Slice& value);

  size_t GetTargetLevel(const size_t now_level, const Meta meta);

  bool RollBack(const size_t now_level, const Meta meta);

  std::vector<Table*> MergeTables(const std::vector<TableIterator*>& tables);

  void CompactList(size_t level);

  void MajorCompact(size_t level);
};
}

#endif