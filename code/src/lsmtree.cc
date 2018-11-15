#include "lsmtree.h"

namespace bilsmtree {

LSMTree::LSMTree(FileSystem* filesystem, LSMTreeResult* lsmtreeresult) {
  filesystem_ = filesystem;
  lsmtreeresult_ = lsmtreeresult;
  total_sequence_number_ = 0;
  recent_files_ = new VisitFrequency(Config::VisitFrequencyConfig::MAXQUEUESIZE, filesystem);
}

LSMTree::~LSMTree() {
  delete recent_files_;
}

bool LSMTree::Get(const Slice key, Slice& value) {
  for (size_t i = 0; i < Config::LSMTreeConfig::LEVEL; ++ i) {
    std::vector<size_t> check_files_;
    if (i == 0) {
      for (size_t j = 0; j < file_[i].size(); ++ j) {
        if (key.compare(file_[i][j].smallest_) >= 0 && key.compare(file_[i][j].largest_) <= 0)
          check_files_.push_back(j);
      }
    }
    else {
      if (file_[i].size() > 0) {
        size_t l = 0;
        size_t r = file_[i].size() - 1;
        if (key.compare(file_[i][r].largest_) > 0)
          continue;
        if (key.compare(file_[i][l].smallest_) < 0)
          continue;
        // (l, r] <= largest_
        while (r - l > 1) {
          size_t m = (l + r) / 2;
          if (key.compare(file_[i][m].largest_) <= 0)
            r = m;
          else
            l = m;
        }
        if (key.compare(file_[i][r].smallest_) >= 0)
          check_files_.push_back(r);
      }
    }
    for (size_t j = 0; j < list_[i].size(); ++ j) {
      if (key.compare(list_[i][j].smallest_) >= 0 && key.compare(list_[i][j].largest_) <= 0)
        check_files_.push_back(j + file_[i].size());
    }
    for (size_t j = 0; j < check_files_.size(); ++ j) {
      size_t p = check_files_[j];
      Meta meta = (p < file_[i].size() ? file_[i][p] : list_[i][p - file_[i].size()]);
      if (GetValueFromFile(meta, key, value)) {
        int deleted = recent_files_->Append(meta.sequence_number_);
        frequency_[meta.sequence_number_] ++;
        if (deleted != -1) frequency_[deleted] --;
        // TODO: RUN BY ALPHA
        if (RollBack(i, meta)) {
          if (p >= file_[i].size())
            list_[i].erase(list_[i].begin() + p - file_[i].size());
          else
            file_[i].erase(file_[i].begin() + p);
        }
        lsmtreeresult_->Check(j + 1);
        return true;
      }
    }
  }
  return false;
}

void LSMTree::AddTableToL0(const std::vector<KV>& kvs) {
  Table *table_ = new Table(kvs, filesystem_);
  size_t sequence_number_ = GetSequenceNumber();
  std::string filename = GetFilename(sequence_number_);
  table_->DumpToFile(filename, lsmtreeresult_);
  Meta meta = table_->GetMeta();
  meta.sequence_number_ = sequence_number_;
  file_[0].insert(file_[0].begin(), meta);
  lsmtreeresult_->MinorCompaction();
  if (file_[0].size() > Config::LSMTreeConfig::L0SIZE)
    MajorCompaction(0);
}

size_t LSMTree::GetSequenceNumber() {
  return total_sequence_number_ ++;
}

std::string LSMTree::GetFilename(size_t sequence_number_) {
  char filename[100];
  sprintf(filename, "%s/%s_%zu.bdb", Config::TableConfig::TABLEPATH, Config::TableConfig::TABLENAME, sequence_number_);
  return std::string(filename);
}

bool LSMTree::GetValueFromFile(const Meta meta, const Slice key, Slice& value) {
  std::string filename = GetFilename(meta.sequence_number_);
  size_t file_number_ = filesystem_->Open(filename, Config::FileSystemConfig::READ_OPTION);
  filesystem_->Seek(file_number_, meta.file_size_ - Config::TableConfig::FOOTERSIZE);
  std::stringstream ss(filesystem_->Read(file_number_, 2 * sizeof(size_t)));
  lsmtreeresult_->Read();
  char *buf = new char[sizeof(size_t)];
  ss.read(buf, sizeof(size_t));
  buf[sizeof(size_t)] = '\0';
  size_t index_offset_ = Util::StringToInt(std::string(buf));
  ss.read(buf, sizeof(size_t));
  buf[sizeof(size_t)] = '\0';
  size_t filter_offset_ = Util::StringToInt(std::string(buf));
  // READ FILTER
  filesystem_->Seek(file_number_, filter_offset_);
  std::string filter_data_ = filesystem_->Read(file_number_, Config::TableConfig::FILTERSIZE);
  lsmtreeresult_->Read();
  Filter* filter;
  std::string algorithm = Util::GetAlgorithm();
  if (algorithm == std::string("LevelDB")) {
    filter = new BloomFilter(filter_data_);
  }
  else if (algorithm == std::string("BiLSMTree")) {
    filter = new CuckooFilter(filter_data_);
  }
  else {
    filter = NULL;
    assert(false);
  }
  if (!filter->KeyMatch(key)) {
    filesystem_->Close(file_number_);
    return false;
  }
  // READ INDEX BLOCK
  filesystem_->Seek(file_number_, index_offset_);
  std::string index_data_ = filesystem_->Read(file_number_, Config::TableConfig::BLOCKSIZE);
  lsmtreeresult_->Read();
  std::istringstream is(index_data_);
  std::string key_;
  int offset_ = -1;
  char temp[1000];
  while (!is.eof()) {
    is.read(temp, sizeof(size_t));
    size_t key_size_ = Util::StringToInt(std::string(temp));
    is.read(temp, key_size_);
    key_ = std::string(temp);
    is.read(temp, sizeof(size_t));
    offset_ = Util::StringToInt(std::string(temp));
    if (key.compare(key_) <= 0)
      break;
  }
  if (offset_ == -1) {
    filesystem_->Close(file_number_);
    return false;
  }
  // READ DATA BLOCK
  filesystem_->Seek(file_number_, offset_);
  std::string data_ = filesystem_->Read(file_number_, Config::TableConfig::BLOCKSIZE);
  lsmtreeresult_->Read();
  filesystem_->Close(file_number_);
  is.str(data_);
  std::string value_;
  while (!is.eof()) {
    is.read(temp, sizeof(size_t));
    size_t key_size_ = Util::StringToInt(std::string(temp));
    is.read(temp, key_size_);
    key_ = std::string(temp);
    is.read(temp, sizeof(size_t));
    size_t value_size_ = Util::StringToInt(std::string(temp));
    is.read(temp, value_size_);
    value_ = std::string(temp);
    if (key.compare(key_) == 0) {
      value = Slice(value_);
      return true;
    }
  }
  return false;
}

size_t LSMTree::GetTargetLevel(const size_t now_level, const Meta meta) {
  size_t overlaps[7];
  overlaps[now_level] = 0;
  for (int i = now_level - 1; i >= 0; -- i) {
    overlaps[i] = overlaps[i  + 1];
    for (size_t j = 0; j < file_[i].size(); ++ j) {
      if (meta.Intersect(file_[i][j]))
        overlaps[i] = overlaps[i] + 1;
    }
  }
  size_t target_level = now_level;
  double diff = 0;
  for (int i = now_level - 1; i > 0; -- i) {
    double diff_t = frequency_[meta.sequence_number_] * 3 * (now_level - i) - Config::FlashConfig::READ_WRITE_RATE - overlaps[i];
    if (diff_t > diff) {
      target_level = i;
      diff = diff_t;
    }
  }
  return target_level;
}

bool LSMTree::RollBack(const size_t now_level, const Meta meta) {
  if (now_level == 0)
    return false;
  size_t to_level = GetTargetLevel(now_level, meta);
  if (to_level == now_level)
    return false;
  std::string filename = GetFilename(meta.sequence_number_);
  size_t file_number_ = filesystem_->Open(filename, Config::FileSystemConfig::READ_OPTION);
  filesystem_->Seek(file_number_, meta.file_size_ - Config::TableConfig::FOOTERSIZE);
  std::stringstream ss(filesystem_->Read(file_number_, 2 * sizeof(size_t)));
  lsmtreeresult_->Read();
  char *buf = new char[sizeof(size_t)];
  ss.read(buf, sizeof(size_t));
  ss.read(buf, sizeof(size_t));
  buf[sizeof(size_t)] = '\0';
  size_t filter_offset_ = Util::StringToInt(std::string(buf));
  // MEGER FILTER
  filesystem_->Seek(file_number_, filter_offset_);
  std::string filter_data_ = filesystem_->Read(file_number_, Config::TableConfig::FILTERSIZE);
  lsmtreeresult_->Read();
  std::string algorithm = Util::GetAlgorithm();
  assert(algorithm == std::string("BiLSMTree"));
  CuckooFilter *filter = new CuckooFilter(filter_data_);
  for (int i = now_level - 1; i >= static_cast<int>(to_level); -- i) {
    int l = 0;
    int r = static_cast<int>(file_[i].size()) - 1;
    while (l < file_[i].size() && meta.smallest_.compare(file_[i][l].largest_) > 0) ++ l;
    while (r >= 0 && meta.largest_.compare(file_[i][l].smallest_) < 0) -- l;
    if (l > r) 
      continue;
    for (int j = l; j <= r; ++ j) {
      Meta to_meta = file_[i][j];
      std::string to_filename = GetFilename(to_meta.sequence_number_);
      size_t to_file_number_ = filesystem_->Open(to_filename, Config::FileSystemConfig::READ_OPTION);
      filesystem_->Seek(to_file_number_, to_meta.file_size_ - Config::TableConfig::FOOTERSIZE);
      ss.str(filesystem_->Read(file_number_, 2 * sizeof(size_t)));
      lsmtreeresult_->Read();
      char *buf = new char[sizeof(size_t)];
      ss.read(buf, sizeof(size_t));
      ss.read(buf, sizeof(size_t));
      buf[sizeof(size_t)] = '\0';
      size_t to_filter_offset_ = Util::StringToInt(std::string(buf));
      filesystem_->Seek(to_file_number_, to_filter_offset_);
      std::string to_filter_data_ = filesystem_->Read(to_file_number_, Config::TableConfig::FILTERSIZE);
      lsmtreeresult_->Read();
      CuckooFilter *to_filter = new CuckooFilter(to_filter_data_);
      filter->Diff(to_filter);
      filesystem_->Close(to_file_number_);
    }
  }
  filesystem_->Seek(file_number_, filter_offset_);
  filter_data_ = filter->ToString();
  filesystem_->Write(file_number_, filter_data_.data(), filter_data_.size());
  filesystem_->Close(file_number_);
  lsmtreeresult_->Write();
  // add to list_
  list_[to_level].push_back(meta);
  if (list_[to_level].size() >= Config::LSMTreeConfig::LISTSIZE)
    CompactList(to_level);
  return true;
}

std::vector<Table*> LSMTree::MergeTables(const std::vector<TableIterator*>& tables) {
  std::vector<KV> buffers_;
  std::vector<Table*> result_;
  size_t size_ = 0;
  // TODO
  std::priority_queue<TableIterator*> q;
  for (size_t i = 0; i < tables.size(); ++ i) {
    TableIterator* ti = tables[i];
    ti->SetId(i);
    q.push(ti);
  }
  while (!q.empty()) {
    TableIterator* ti = q.top();
    q.pop();
    if (ti->HasNext()) {
      KV kv = ti->Next();
      q.push(ti);
      if (buffers_.size() == 0 || kv.key_.compare(buffers_[buffers_.size() - 1].key_) > 0) {
        if (size_ + kv.size() > Config::TableConfig::TABLESIZE) {
          Table* t = new Table(buffers_, filesystem_);
          result_.push_back(t);
          buffers_.clear();
          size_ = 0;
        }
        buffers_.push_back(kv);
      }
    }
  }
  if (size_ > 0) {
    Table* t = new Table(buffers_, filesystem_);
    result_.push_back(t);
  }
  return result_;
}

void LSMTree::CompactList(size_t level) {
  std::vector<TableIterator*> tables_;
  for (size_t i = 0; i < list_[level].size(); ++ i) {
    TableIterator* t = new TableIterator(GetFilename(list_[level][i].sequence_number_), filesystem_);
    tables_.push_back(t);
  }
  std::vector<Table*> merged_tables = MergeTables(tables_);
  for (size_t i = 0; i < merged_tables.size(); ++ i) {
    size_t sequence_number_ = GetSequenceNumber();
    std::string filename = GetFilename(sequence_number_);
    merged_tables[i]->DumpToFile(filename, lsmtreeresult_);
    Meta meta = merged_tables[i]->GetMeta();
    meta.sequence_number_ = sequence_number_;
    list_[level].push_back(meta);
  }
  if (list_[level].size() >= Config::LSMTreeConfig::LISTSIZE) {
    // TODO
    std::cout << "LIST IS TOO LARGE" << std::endl;
  }
}

void LSMTree::MajorCompaction(size_t level) {
  if (level == Config::LSMTreeConfig::LEVEL)
    return ;
  std::vector<Meta> metas;
  // select a file from Li
  // select from file_
  size_t index = 0;
  for (size_t i = 1; i < file_[level].size(); ++ i) {
    Meta m = file_[level][i];
    size_t last_seq_ = file_[level][index].sequence_number_;
    if (frequency_[m.sequence_number_] < frequency_[last_seq_]) 
      index = i;
  }
  // select from list_
  for (size_t i = 0; i < list_[level].size(); ++ i) {
    Meta m = list_[level][i];
    size_t last_seq_ = (index >= file_[level].size() ? list_[level][index - file_[level].size()].sequence_number_ : file_[level][index].sequence_number_);
    if (frequency_[m.sequence_number_] < frequency_[last_seq_])
      index = i + file_[level].size();
  }
  // push meta
  if (index >= file_[level].size()) {
    index = index - file_[level].size();
    metas.push_back(list_[level][index]);
    list_[level].erase(list_[level].begin() + index);
  }
  else {
    metas.push_back(file_[level][index]);
    file_[level].erase(file_[level].begin() + index);
  }
  // select overlap files
  // select from file_
  Meta meta = metas[0];
  for (size_t i = 0; i < file_[level + 1].size(); ) {
    if (file_[level + 1][i].Intersect(meta)) {
      metas.push_back(file_[level + 1][i]);
      file_[level + 1].erase(file_[level + 1].begin() + i);
    }
    else {
      ++ i;
    }
  }
  // select from list_
  for (size_t i = 0; i < list_[level + 1].size(); ) {
    if (list_[level + 1][i].Intersect(meta)) {
      metas.push_back(list_[level + 1][i]);
      list_[level + 1].erase(list_[level + 1].begin() + i);
    }
    else {
      ++ i;
    }
  }
  std::vector<TableIterator*> tables_;
  for (size_t i = 0; i < metas.size(); ++ i) {
    TableIterator* t = new TableIterator(GetFilename(metas[i].sequence_number_), filesystem_);
    tables_.push_back(t);
  }
  std::vector<Table*> merged_tables = MergeTables(tables_);
  for (size_t i = 0; i < merged_tables.size(); ++ i) {
    size_t sequence_number_ = GetSequenceNumber();
    std::string filename = GetFilename(sequence_number_);
    merged_tables[i]->DumpToFile(filename, lsmtreeresult_);
    Meta meta = merged_tables[i]->GetMeta();
    meta.sequence_number_ = sequence_number_;
    file_[level + 1].push_back(meta);
  }
  
  std::sort(file_[level + 1].begin(), file_[level + 1].end(), [](const Meta& a, const Meta& b)->bool { 
    if (a.largest_.compare(b.largest_) != 0)
      return a.largest_.compare(b.largest_) <= 0;
    return a.smallest_.compare(b.smallest_) <= 0;
  });
  lsmtreeresult_->MajorCompaction();
  if (file_[level + 1].size() > static_cast<size_t>(pow(10, level)))
    MajorCompaction(level + 1);
}

}