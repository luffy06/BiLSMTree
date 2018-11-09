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

  size_t Id() { return id_;}

  KV Next();

  KV Current();

  void SetId(size_t id);
private:
  size_t id_;
  std::vector<KV> kvs_;
  size_t iter_;
};
}

#endif