#include "lsmtree.h"

namespace bilsmtree {

LSMTree::LSMTree() {
  total_sequence_number_ = 0;
  recent_files_ = new VisitFrequency();
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
        return true;
      }
    }
  }
  return false;
}

void LSMTree::AddTableToL0(const std::vector<KV>& kvs) {
  Table *table_ = new Table(kvs);
  size_t sequence_number_ = GetSequenceNumber();
  std::string filename = GetFilename(sequence_number_);
  table_->DumpToFile(filename);
  Meta meta = table_->GetMeta();
  meta.sequence_number_ = sequence_number_;
  file_[0].insert(file_[0].begin(), meta);
  if (file_[0].size() > Config::LSMTreeConfig::L0SIZE)
    MajorCompact(0);
}

size_t LSMTree::GetSequenceNumber() {
  return total_sequence_number_ ++;
}

std::string LSMTree::GetFilename(size_t sequence_number_) {
  char filename[30];
  sprintf(filename, "%s/%s_%zu.bdb", Config::TableConfig::TABLEPATH, Config::TableConfig::TABLENAME, sequence_number_);
  return std::string(filename);
}

bool LSMTree::GetValueFromFile(const Meta meta, const Slice key, Slice& value) {
  std::string filename = GetFilename(meta.sequence_number_);
  size_t file_number_ = FileSystem::Open(filename, Config::FileSystemConfig::READ_OPTION);
  FileSystem::Seek(file_number_, meta.file_size_ - Config::TableConfig::FOOTERSIZE);
  size_t index_offset_ = static_cast<size_t>(Util::StringToInt(FileSystem::Read(file_number_, sizeof(size_t))));
  size_t filter_offset_ = static_cast<size_t>(Util::StringToInt(FileSystem::Read(file_number_, sizeof(size_t))));
  // READ FILTER
  FileSystem::Seek(file_number_, filter_offset_);
  std::string filter_data_ = FileSystem::Read(file_number_, Config::TableConfig::FILTERSIZE);
  Filter* filter;
  if (Config::algorithm == Config::LevelDB)
    filter = new BloomFilter(filter_data_);
  else if (Config::algorithm == Config::BiLSMTree)
    filter = new CuckooFilter(filter_data_);
  else
    Util::Assert("Algorithm Error", false);
  if (!filter->KeyMatch(key)) {
    FileSystem::Close(file_number_);
    return false;
  }
  // READ INDEX BLOCK
  FileSystem::Seek(file_number_, index_offset_);
  std::string index_data_ = FileSystem::Read(file_number_, Config::TableConfig::BLOCKSIZE);
  std::istringstream is(index_data_);
  std::string key_;
  size_t offset_ = -1;
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
    FileSystem::Close(file_number_);
    return false;
  }
  // READ DATA BLOCK
  FileSystem::Seek(file_number_, offset_);
  std::string data_ = FileSystem::Read(file_number_, Config::TableConfig::BLOCKSIZE);
  FileSystem::Close(file_number_);
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
  size_t file_number_ = FileSystem::Open(filename, Config::FileSystemConfig::READ_OPTION);
  FileSystem::Seek(file_number_, meta.file_size_ - Config::TableConfig::FOOTERSIZE);
  size_t index_offset_ = static_cast<size_t>(Util::StringToInt(FileSystem::Read(file_number_, sizeof(size_t))));
  size_t filter_offset_ = static_cast<size_t>(Util::StringToInt(FileSystem::Read(file_number_, sizeof(size_t))));
  // MEGER FILTER
  FileSystem::Seek(file_number_, filter_offset_);
  std::string filter_data_ = FileSystem::Read(file_number_, Config::TableConfig::FILTERSIZE);
  Util::Assert("Algorithm Error", Config::algorithm == Config::BiLSMTree);
  CuckooFilter *filter = new CuckooFilter(filter_data_);
  for (int i = now_level - 1; i >= to_level; -- i) {
    size_t l = 0;
    int r = file_[i].size() - 1;
    while (l < file_[i].size() && meta.smallest_.compare(file_[i][l].largest_) > 0) ++ l;
    while (r >= 0 && meta.largest_.compare(file_[i][l].smallest_) < 0) -- l;
    if (l > r) 
      continue;
    for (size_t j = l; j <= r; ++ j) {
      Meta to_meta = file_[i][j];
      std::string to_filename = GetFilename(to_meta.sequence_number_);
      int to_file_number_ = FileSystem::Open(to_filename, Config::FileSystemConfig::READ_OPTION);
      FileSystem::Seek(to_file_number_, to_meta.file_size_ - Config::TableConfig::FOOTERSIZE);
      size_t to_index_offset_ = static_cast<size_t>(Util::StringToInt(FileSystem::Read(to_file_number_, sizeof(size_t))));
      size_t to_filter_offset_ = static_cast<size_t>(Util::StringToInt(FileSystem::Read(to_file_number_, sizeof(size_t))));
      FileSystem::Seek(to_file_number_, to_filter_offset_);
      std::string to_filter_data_ = FileSystem::Read(to_file_number_, Config::TableConfig::FILTERSIZE);
      CuckooFilter *to_filter = new CuckooFilter(to_filter_data_);
      filter->Diff(to_filter);
      FileSystem::Close(to_file_number_);
    }
  }
  FileSystem::Seek(file_number_, filter_offset_);
  filter_data_ = filter->ToString();
  FileSystem::Write(file_number_, filter_data_.data(), filter_data_.size());
  FileSystem::Close(file_number_);
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
          Table* t = new Table(buffers_);
          result_.push_back(t);
          buffers_.clear();
          size_ = 0;
        }
        buffers_.push_back(kv);
      }
    }
  }
  if (size_ > 0) {
    Table* t = new Table(buffers_);
    result_.push_back(t);
  }
  return result_;
}

void LSMTree::CompactList(size_t level) {
  std::vector<TableIterator*> tables_;
  for (size_t i = 0; i < list_[level].size(); ++ i) {
    TableIterator* t = new TableIterator(GetFilename(list_[level][i].sequence_number_));
    tables_.push_back(t);
  }
  std::vector<Table*> merged_tables = MergeTables(tables_);
  for (size_t i = 0; i < merged_tables.size(); ++ i) {
    size_t sequence_number_ = GetSequenceNumber();
    std::string filename = GetFilename(sequence_number_);
    merged_tables[i]->DumpToFile(filename);
    Meta meta = merged_tables[i]->GetMeta();
    meta.sequence_number_ = sequence_number_;
    list_[level].push_back(meta);
  }
  if (list_[level].size() >= Config::LSMTreeConfig::LISTSIZE) {
    // TODO
    std::cout << "LIST IS TOO LARGE" << std::endl;
  }
}

void LSMTree::MajorCompact(size_t level) {
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
    TableIterator* t = new TableIterator(GetFilename(metas[i].sequence_number_));
    tables_.push_back(t);
  }
  std::vector<Table*> merged_tables = MergeTables(tables_);
  for (size_t i = 0; i < merged_tables.size(); ++ i) {
    size_t sequence_number_ = GetSequenceNumber();
    std::string filename = GetFilename(sequence_number_);
    merged_tables[i]->DumpToFile(filename);
    Meta meta = merged_tables[i]->GetMeta();
    meta.sequence_number_ = sequence_number_;
    file_[level + 1].push_back(meta);
  }
  
  std::sort(file_[level + 1].begin(), file_[level + 1].end(), [](const Meta& a, const Meta& b)->bool { 
    if (a.largest_.compare(b.largest_) != 0)
      return a.largest_.compare(b.largest_) <= 0;
    return a.smallest_.compare(b.smallest_) <= 0;
  });

  if (file_[level + 1].size() > static_cast<size_t>(pow(10, level)))
    MajorCompact(level + 1);
}

}