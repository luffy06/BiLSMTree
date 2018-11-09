#ifndef BILSMTREE_TABLEITERATOR_H
#define BILSMTREE_TABLEITERATOR_H

#include <vector>
#include <string>
#include <sstream>

#include "slice.h"
#include "filesystem.h"

namespace bilsmtree {

struct KV;
class FileSystem;

class TableIterator {
public:
  TableIterator();

  TableIterator(const std::string filename);

  ~TableIterator();
  
  void ParseBlock(const std::string block_data);

  bool HasNext();

  uint Id() { return id_;}

  KV Next();

  KV Current();

  void SetId(uint id);
private:
  uint id_;
  std::vector<KV> kvs_;
  uint iter_;
};
}

#endif