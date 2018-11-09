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

class VisitFrequency;
class TableIterator;
class Table;

class LSMTree {
public:
  LSMTree();
  ~LSMTree();

  bool Get(const Slice key, Slice& value);

  void AddTableToL0(const std::vector<KV>& kvs);

  uint GetSequenceNumber();
private:
  std::vector<Meta> file_[Config::LSMTreeConfig::LEVEL];
  std::vector<Meta> list_[Config::LSMTreeConfig::LEVEL];
  VisitFrequency *recent_files_;
  uint min_frequency_number_[Config::LSMTreeConfig::LEVEL];
  std::vector<uint> frequency_;
  uint total_sequence_number_;

  std::string GetFilename(uint sequence_number_);

  bool GetValueFromFile(const Meta meta, const Slice key, Slice& value);

  uint GetTargetLevel(const uint now_level, const Meta meta);

  bool RollBack(const uint now_level, const Meta meta);

  std::vector<Table*> MergeTables(const std::vector<TableIterator*>& tables);

  void CompactList(uint level);

  void MajorCompact(uint level);
};
}

#endif