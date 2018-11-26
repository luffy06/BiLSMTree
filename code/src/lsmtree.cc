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
  // std::cout << "LSMTree Get:" << key.ToString() << std::endl;
  // for (size_t i = 0; i < Config::LSMTreeConfig::LEVEL; ++ i) {
  //   std::cout << "Level:" << i << std::endl;
  //   std::cout << "file_: " << file_[i].size() << std::endl;
  //   std::cout << "list_: " << list_[i].size() << std::endl;    
  //   for (size_t j = 0; j < file_[i].size(); ++ j)
  //     std::cout << "file_:" << file_[i][j].smallest_.ToString() << "\t" << file_[i][j].largest_.ToString() << "\tFileSize:" << file_[i][j].file_size_ << std::endl;
  //   for (size_t j = 0; j < list_[i].size(); ++ j)
  //     std::cout << "list_:" << list_[i][j].smallest_.ToString() << "\t" << list_[i][j].largest_.ToString() << "\tFileSize:" << list_[i][j].file_size_ << std::endl;
  // }
  std::string algo = Util::GetAlgorithm();
  size_t checked = 0;
  for (size_t i = 0; i < Config::LSMTreeConfig::LEVEL; ++ i) {
    // std::cout << "Level " << i << std::endl;
    // std::cout << "file_: " << file_[i].size() << std::endl;
    // std::cout << "list_: " << list_[i].size() << std::endl;
    std::vector<size_t> check_files_;
    if (i == 0) {
      for (size_t j = 0; j < file_[i].size(); ++ j) {
        // std::cout << "file_:" << file_[i][j].smallest_.ToString() << "\t" << file_[i][j].largest_.ToString() << std::endl;
        if (key.compare(file_[i][j].smallest_) >= 0 && key.compare(file_[i][j].largest_) <= 0) {
          // std::cout << "file_:" << file_[i][j].smallest_.ToString() << "\t" << file_[i][j].largest_.ToString() << std::endl;
          check_files_.push_back(j);
        }
      }
    }
    else {
      if (file_[i].size() > 0) {
        size_t l = 0;
        size_t r = file_[i].size() - 1;
        // std::cout << "file_ of l:" << file_[i][l].smallest_.ToString() << "\t" << file_[i][l].largest_.ToString() << std::endl;
        // std::cout << "file_ of r:" << file_[i][r].smallest_.ToString() << "\t" << file_[i][r].largest_.ToString() << std::endl;
        if (key.compare(file_[i][r].largest_) <= 0 && key.compare(file_[i][l].smallest_) >= 0) {
          if (key.compare(file_[i][l].largest_) <= 0) {
            r = l;
          }
          else {
            // std::cout << "Start Binary Search" << std::endl;
            // (l, r] <= largest_
            while (r - l > 1) {
              size_t m = (l + r) / 2;
              if (key.compare(file_[i][m].largest_) <= 0)
                r = m;
              else
                l = m;
            }
          }
          if (key.compare(file_[i][r].smallest_) >= 0)
            check_files_.push_back(r);
        }
      }
    }
    if (algo == std::string("BiLSMTree")) {
      // std::cout << "List Size of " << i << ":" << list_[i].size() << std::endl;
      for (size_t j = 0; j < list_[i].size(); ++ j) {
        // std::cout << "list_:" << list_[i][j].smallest_.ToString() << "\t" << list_[i][j].largest_.ToString() << std::endl;
        if (key.compare(list_[i][j].smallest_) >= 0 && key.compare(list_[i][j].largest_) <= 0) {
          // std::cout << "list_:" << list_[i][j].smallest_.ToString() << "\t" << list_[i][j].largest_.ToString() << std::endl;
          check_files_.push_back(j + file_[i].size());
        }
      }
    }
    // std::cout << "check_files_: " << check_files_.size() << std::endl;
    for (size_t j = 0; j < check_files_.size(); ++ j) {
      size_t p = check_files_[j];
      checked = checked + 1;
      Meta meta = (p < file_[i].size() ? file_[i][p] : list_[i][p - file_[i].size()]);
      // std::cout << "check:" << meta.smallest_.ToString() << "\t" << meta.largest_.ToString() << std::endl;
      if (GetValueFromFile(meta, key, value)) {
        if (algo == std::string("BiLSMTree")) {
          int deleted = recent_files_->Append(meta.sequence_number_);
          frequency_[meta.sequence_number_] ++;
          if (deleted != -1) frequency_[deleted] --;
          // TODO: RUN BY ALPHA
          if (i > 0) {
            size_t high_fre = frequency_[file_[i - 1][0].sequence_number_];
            for (size_t k = 1; k < file_[i - 1].size(); ++ k) {
              if (frequency_[file_[i - 1][k].sequence_number_] > high_fre)
                high_fre = frequency_[file_[i - 1][k].sequence_number_];
            }
            std::cout << "ROLLBACK:" << frequency_[meta.sequence_number_] << "\t" << high_fre << std::endl;
            if (frequency_[meta.sequence_number_] >= high_fre * (1. + Config::LSMTreeConfig::ALPHA))
              RollBack(i, meta, p);
          }
        }
        lsmtreeresult_->Check(checked);
        return true;
      }
    }
  }
  return false;
}

void LSMTree::AddTableToL0(const std::vector<KV>& kvs) {
  // std::cout << "Data to be DUMP" << std::endl;
  // for (size_t i = 0; i < kvs.size(); ++ i)
  //   std::cout << kvs[i].key_.ToString() << "\t";
  // std::cout << std::endl;
  if (Config::TRACE_LOG)
    std::cout << "Create Table" << std::endl;
  Table *table_ = new Table(kvs, filesystem_);
  size_t sequence_number_ = GetSequenceNumber();
  std::string filename = GetFilename(sequence_number_);
  if (Config::TRACE_LOG)
    std::cout << "Table DumpToFile" << std::endl;
  table_->DumpToFile(filename, lsmtreeresult_);
  Meta meta = table_->GetMeta();
  if (Config::TRACE_LOG)
    std::cout << "DUMP L0:" << filename << std::endl;
  meta.sequence_number_ = sequence_number_;
  file_[0].insert(file_[0].begin(), meta);
  lsmtreeresult_->MinorCompaction();
  delete table_;
  std::string algo = Util::GetAlgorithm();
  if (algo == std::string("BiLSMTree")) {
    int deleted = recent_files_->Append(sequence_number_);
    frequency_[sequence_number_] ++;
    if (deleted != -1) frequency_[deleted] --;
  }
  if (file_[0].size() > Config::LSMTreeConfig::L0SIZE) {
    // std::cout << "Start MajorCompaction In AddTableToL0" << std::endl;
    MajorCompaction(0);
  }
}

size_t LSMTree::GetSequenceNumber() {
  frequency_.push_back(0);
  return total_sequence_number_ ++;
}

std::string LSMTree::GetFilename(size_t sequence_number_) {
  char filename[100];
  sprintf(filename, "%s%s_%zu.bdb", Config::TableConfig::TABLEPATH, Config::TableConfig::TABLENAME, sequence_number_);
  return std::string(filename);
}

bool LSMTree::GetValueFromFile(const Meta meta, const Slice key, Slice& value) {
  // std::cout << "GetValueFromFile Key:" << key.ToString() << std::endl;
  std::string filename = GetFilename(meta.sequence_number_);
  std::stringstream ss;
  // std::cout << "SEQ:" << meta.sequence_number_ << "\tfile size:" << meta.file_size_ << std::endl;
  size_t file_number_ = filesystem_->Open(filename, Config::FileSystemConfig::READ_OPTION);
  if (Config::SEEK_LOG)
    std::cout << "Seek Footer" << std::endl;
  filesystem_->Seek(file_number_, meta.file_size_ - meta.footer_size_);
  std::string offset_data_ = filesystem_->Read(file_number_, meta.footer_size_);
  ss.str(offset_data_);
  lsmtreeresult_->Read();
  size_t index_offset_ = 0;
  size_t filter_offset_ = 0;
  ss >> index_offset_;
  ss >> filter_offset_;
  // std::cout << "IndexOffset:" << index_offset_ << "\tFilterOffset:" << filter_offset_ << std::endl;
  assert(index_offset_ != 0);
  assert(filter_offset_ != 0);
  // READ FILTER
  if (Config::SEEK_LOG)
    std::cout << "Seek Filter" << std::endl;
  filesystem_->Seek(file_number_, filter_offset_);
  std::string filter_data_ = filesystem_->Read(file_number_, meta.file_size_ - filter_offset_ - meta.footer_size_);
  lsmtreeresult_->Read();
  Filter* filter;
  std::string algo = Util::GetAlgorithm();
  if (algo == std::string("LevelDB") || algo == std::string("LevelDB-KV")) {
    filter = new BloomFilter(filter_data_);
  }
  else if (algo == std::string("BiLSMTree")) {
    filter = new CuckooFilter(filter_data_);
  }
  else {
    filter = NULL;
    assert(false);
  }
  // std::cout << "Filter KeyMatch" << std::endl;
  if (!filter->KeyMatch(key)) {
    // std::cout << filter_data_ << std::endl;
    // std::cout << "Filter Doesn't Exist" << std::endl;
    filesystem_->Close(file_number_);
    return false;
  }
  // READ INDEX BLOCK
  if (Config::SEEK_LOG)
    std::cout << "Seek Index" << std::endl;
  filesystem_->Seek(file_number_, index_offset_);
  std::string index_data_ = filesystem_->Read(file_number_, filter_offset_ - index_offset_);
  // std::cout << "Index Data:" << index_data_ << std::endl;
  lsmtreeresult_->Read();
  ss.str(index_data_);
  bool found = false;
  size_t offset_ = 0;
  size_t data_block_size_ = 0;
  while (true) {
    size_t key_size_ = 0;
    ss >> key_size_;
    ss.get();
    char *key_ = new char[key_size_ + 1];
    ss.read(key_, sizeof(char) * key_size_);
    key_[key_size_] = '\0';
    ss >> offset_ >> data_block_size_;
    if (ss.tellg() == -1)
      break;
    // std::cout << "Index:" << key_size_ << "\t" << key_ << "\t" << offset_ << "\t" << data_block_size_ << std::endl;
    if (key.compare(Slice(key_, key_size_)) <= 0) {
      found = true;
      break;
    }
  }
  if (!found) {
    filesystem_->Close(file_number_);
    return false;
  }
  // READ DATA BLOCK
  if (Config::SEEK_LOG)
    std::cout << "Seek Data" << std::endl;
  filesystem_->Seek(file_number_, offset_);
  std::string data_ = filesystem_->Read(file_number_, data_block_size_);
  // std::cout << "Find Data Block:" << data_ << std::endl;
  lsmtreeresult_->Read();
  filesystem_->Close(file_number_);
  ss.str(data_);
  while (true) {
    size_t key_size_ = 0;
    ss >> key_size_;
    char *key_ = new char[key_size_ + 1];
    ss.get();
    ss.read(key_, sizeof(char) * key_size_);
    key_[key_size_] = '\0';

    size_t value_size_ = 0;
    ss >> value_size_;
    char *value_ = new char[value_size_ + 1];
    ss.get();
    ss.read(value_, sizeof(char) * value_size_);
    value_[value_size_] = '\0';

    if (ss.tellg() == -1)
      break;

    // std::cout << "Key:" << key_ << "\tValue:" << value_ << std::endl;
    if (key.compare(Slice(key_, key_size_)) == 0) {
      // std::cout << "Find!!" << value_ << std::endl;
      value = Slice(value_, value_size_);
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
  for (int i = now_level - 1; i >= 0; -- i) {
    double diff_t = frequency_[meta.sequence_number_] * 3 * (now_level - i) * (Config::LSMTreeConfig::LISTSIZE + 1) - Config::FlashConfig::READ_WRITE_RATE - overlaps[i] - 1;
    if (diff_t > diff) {
      target_level = i;
      diff = diff_t;
    }
  }
  return target_level;
}

void LSMTree::RollBack(const size_t now_level, const Meta meta, const size_t pos) {
  // std::cout << "Ready RollBack" << std::endl;
  if (now_level == 0)
    return ;
  size_t to_level = GetTargetLevel(now_level, meta);
  assert(to_level <= now_level);
  if (to_level == now_level)
    return ;
  if (Config::TRACE_LOG) {
    std::cout << "RollBack Now Level:" << now_level << " To Level:" << to_level << std::endl;
    std::cout << "FileSize:" << meta.file_size_ << " [" << meta.smallest_.ToString().size() << "\t" << meta.largest_.ToString().size() << "]" << std::endl;
  }
  std::stringstream ss;
  std::string filename = GetFilename(meta.sequence_number_);
  size_t file_number_ = filesystem_->Open(filename, Config::FileSystemConfig::READ_OPTION | Config::FileSystemConfig::WRITE_OPTION);
  if (Config::SEEK_LOG)
    std::cout << "Seek Footer in RollBack" << std::endl;
  filesystem_->Seek(file_number_, meta.file_size_ - meta.footer_size_);
  std::string footer_data_ = filesystem_->Read(file_number_, meta.footer_size_);
  // std::cout << "Footer:" << footer_data_ << std::endl;
  ss.str(footer_data_);
  lsmtreeresult_->Read();
  size_t index_offset_ = 0;
  size_t filter_offset_ = 0;
  ss >> index_offset_;
  ss >> filter_offset_;
  // std::cout << "IndexOffset:" << index_offset_ << "\tFilterOffset:" << filter_offset_ << std::endl;
  assert(index_offset_ != 0);
  assert(filter_offset_ != 0);
  // MEGER FILTER
  if (Config::SEEK_LOG)
    std::cout << "Seek Fitler in RollBack" << std::endl;
  filesystem_->Seek(file_number_, filter_offset_);
  std::string filter_data_ = filesystem_->Read(file_number_, meta.file_size_ - filter_offset_ - meta.footer_size_);
  lsmtreeresult_->Read();
  std::string algorithm = Util::GetAlgorithm();
  assert(algorithm == std::string("BiLSMTree"));
  CuckooFilter *filter = new CuckooFilter(filter_data_);
  // std::cout << "Init FilterDataSize:" << filter->ToString().size() << "\t" << filter_data_.size() << std::endl;
  for (int i = now_level - 1; i >= static_cast<int>(to_level); -- i) {
    int l = 0;
    int r = static_cast<int>(file_[i].size()) - 1;
    while (l < static_cast<int>(file_[i].size()) && meta.smallest_.compare(file_[i][l].largest_) > 0) ++ l;
    while (r >= 0 && meta.largest_.compare(file_[i][r].smallest_) < 0) -- r;
    // std::cout << "Overlap Interval: [" << l << ", " << r << "]" << std::endl;
    if (l > r)
      continue;
    for (int j = l; j <= r; ++ j) {
      Meta to_meta = file_[i][j];
      std::string to_filename = GetFilename(to_meta.sequence_number_);
      // std::cout << "SEQ:" << to_meta.sequence_number_ << std::endl;
      size_t to_file_number_ = filesystem_->Open(to_filename, Config::FileSystemConfig::READ_OPTION);
      if (Config::SEEK_LOG)
        std::cout << "Seek ToFile Footer in RollBack" << std::endl;
      filesystem_->Seek(to_file_number_, to_meta.file_size_ - to_meta.footer_size_);
      std::string to_offset_data_ = filesystem_->Read(to_file_number_, to_meta.footer_size_);
      ss.str("");
      ss.str(to_offset_data_);
      lsmtreeresult_->Read();
      size_t to_index_offset_ = 0;
      size_t to_filter_offset_ = 0;
      ss >> to_index_offset_;
      ss >> to_filter_offset_;
      // std::cout << "ToIndexOffset:" << to_index_offset_ << "\tToFilterOffset:" << to_filter_offset_ << std::endl;
      // std::cout << "ToFileSize:" << to_meta.file_size_ << "\tToFooterSize:" << to_meta.footer_size_ << std::endl;
      if (Config::SEEK_LOG)
        std::cout << "Seek ToFile Footer in RollBack" << std::endl;
      filesystem_->Seek(to_file_number_, to_filter_offset_);
      std::string to_filter_data_ = filesystem_->Read(to_file_number_, to_meta.file_size_ - to_filter_offset_ - to_meta.footer_size_);
      lsmtreeresult_->Read();
      CuckooFilter *to_filter = new CuckooFilter(to_filter_data_);
      filter->Diff(to_filter);
      delete to_filter;
      filesystem_->Close(to_file_number_);
      // std::cout << "FilterDataSize:" << filter->ToString().size() << std::endl;
    }
  }
  if (Config::SEEK_LOG)
    std::cout << "Seek Filter for Write in RollBack" << std::endl;
  filesystem_->Seek(file_number_, filter_offset_);
  int file_size_minus_ = filter_data_.size() - filter->ToString().size();
  // std::cout << "Final FilterDataSize:" << filter->ToString().size() << "\t" << filter_data_.size() << std::endl;
  // std::cout << "ReWrite File Footer: " << meta.sequence_number_ << "\t" << file_size_minus_ << std::endl;
  assert(file_size_minus_ >= 0);
  filter_data_ = filter->ToString();
  delete filter;
  filesystem_->Write(file_number_, filter_data_.data(), filter_data_.size());
  filesystem_->Write(file_number_, footer_data_.data(), footer_data_.size());
  filesystem_->SetFileSize(file_number_, meta.file_size_ - file_size_minus_);
  lsmtreeresult_->Write();  
  filesystem_->Close(file_number_);

  Meta new_meta;
  new_meta.Copy(meta);
  new_meta.file_size_ = meta.file_size_ - file_size_minus_;
  new_meta.level_ = to_level;
  // add to list_
  list_[to_level].push_back(new_meta);
  if (pos >= file_[now_level].size())
    list_[now_level].erase(list_[now_level].begin() + pos - file_[now_level].size());
  else
    file_[now_level].erase(file_[now_level].begin() + pos);
  if (list_[to_level].size() >= Config::LSMTreeConfig::LISTSIZE)
    CompactList(to_level);
  if (Config::TRACE_LOG)
    std::cout << "RollBack Success" << std::endl;
}

std::vector<Table*> LSMTree::MergeTables(const std::vector<TableIterator>& tables) {
  if (Config::TRACE_LOG)
    std::cout << "MergeTables Start:" << tables.size() << std::endl;
  std::vector<KV> buffers_;
  std::vector<Table*> result_;
  std::priority_queue<TableIterator> q;
  for (size_t i = 0; i < tables.size(); ++ i) {
    TableIterator ti = tables[i];
    ti.SetId(i);
    if (ti.HasNext())
      q.push(ti);
  }
  while (!q.empty()) {
    TableIterator ti = q.top();
    q.pop();
    KV kv = ti.Next();
    // std::cout << "Add:" << kv.key_.ToString() << "\tfrom:" << ti.Id() << std::endl;
    if (ti.HasNext())
      q.push(ti);
    if (buffers_.size() == 0 || kv.key_.compare(buffers_[buffers_.size() - 1].key_) > 0) {
      if (buffers_.size() >= Config::TableConfig::TABLESIZE) {
        Table *t = new Table(buffers_, filesystem_);
        result_.push_back(t);
        buffers_.clear();
      }
      buffers_.push_back(kv);
    }
  }
  if (buffers_.size() > 0) {
    // std::cout << "Create Table:[" << (*buffers_.begin()).key_.ToString() << "\t" << (*buffers_.rbegin()).key_.ToString() << "]" << std::endl;
    Table *t = new Table(buffers_, filesystem_);
    result_.push_back(t);
  }
  if (Config::TRACE_LOG)
    std::cout << "MergeTables End:" << result_.size() << std::endl;
  return result_;
}

void LSMTree::CompactList(size_t level) {
  if (Config::TRACE_LOG)
    std::cout << "Create TableIterator In CompactList Size:" << list_[level].size() << std::endl;
  std::vector<TableIterator> tables_;
  for (size_t i = 0; i < list_[level].size(); ++ i) {
    std::string filename = GetFilename(list_[level][i].sequence_number_);
    // std::cout << "MERGE Filename:" << filename << " from Level:" << list_[level][i].level_ << "[" << list_[level][i].smallest_.ToString() << "\t" << list_[level][i].largest_.ToString() << "]" << std::endl;
    TableIterator t = TableIterator(filename, filesystem_, list_[level][i], lsmtreeresult_);
    tables_.push_back(t);
    filesystem_->Delete(filename);
  }
  if (Config::TRACE_LOG)
    std::cout << "Create TableIterator In CompactList Success" << std::endl;
  list_[level].clear();
  std::vector<Table*> merged_tables = MergeTables(tables_);
  for (size_t i = 0; i < merged_tables.size(); ++ i) {
    size_t sequence_number_ = GetSequenceNumber();
    std::string filename = GetFilename(sequence_number_);
    merged_tables[i]->DumpToFile(filename, lsmtreeresult_);
    Meta meta = merged_tables[i]->GetMeta();
    meta.sequence_number_ = sequence_number_;
    meta.level_ = level;
    // std::cout << "In List Level:" << meta.level_ << "\tTable:[" << meta.smallest_.ToString() << "\t" << meta.largest_.ToString() << "]" << std::endl;
    list_[level].push_back(meta);
    delete merged_tables[i];
  }
  if (list_[level].size() >= Config::LSMTreeConfig::LISTSIZE) {
    // TODO
    if (Config::TRACE_LOG)
      std::cout << "LIST IS TOO LARGE" << std::endl;
    MajorCompaction(level);
  }
}

void LSMTree::MajorCompaction(size_t level) {
  if (Config::TRACE_LOG)
    std::cout << "MajorCompaction On Level:" << level << std::endl;
  // std::cout << "File Size:" << file_[level + 1].size() << "\tList Size:" << list_[level + 1].size() << std::endl;
  if (level == Config::LSMTreeConfig::LEVEL)
    return ;
  std::vector<Meta> metas;
  std::string algo = Util::GetAlgorithm();
  size_t level_max_file_Li = static_cast<size_t>(pow(10, level));
  if (file_[level].size() <= level_max_file_Li)
    return ;
  size_t select_numb_Li = file_[level].size() - level_max_file_Li;
  // std::cout << "SELECT NUMB:" << select_numb_Li << std::endl;
  // select a file from Li
  // select from file_
  std::vector<size_t> indexs;
  indexs.push_back(0);
  for (size_t i = 1; i < file_[level].size(); ++ i) {
    Meta m = file_[level][i];
    size_t last_index = indexs[indexs.size() - 1];
    if (indexs.size() < select_numb_Li || (indexs.size() == select_numb_Li && frequency_[m.sequence_number_] < frequency_[file_[level][last_index].sequence_number_])) {
      bool add_ = false;
      for (size_t j = 0; j < indexs.size(); ++ j) {
        size_t index = indexs[j];
        if (frequency_[m.sequence_number_] < frequency_[file_[level][index].sequence_number_]) {
          indexs.insert(indexs.begin() + j, i);
          add_ = true;
          break;
        }
      }
      if (!add_)
        indexs.insert(indexs.end(), i);
      if (indexs.size() > select_numb_Li)
        indexs.erase(indexs.begin() + indexs.size() - 1);
    }
  }
  assert(indexs.size() == select_numb_Li);
  std::vector<Meta> temp;
  size_t t = 0;
  sort(indexs.begin(), indexs.end());
  for (size_t i = 0; i < file_[level].size(); ++ i) {
    if (t == indexs.size() ||  i < indexs[t]) {
      temp.push_back(file_[level][i]);
    }
    else {
      metas.push_back(file_[level][indexs[t]]);
      t = t + 1;
    }
  }
  assert(t == indexs.size());
  file_[level].clear();
  for (size_t i = 0; i < temp.size(); ++ i)
    file_[level].push_back(temp[i]);
  if (algo == std::string("BiLSMTree")) {
    // select from list_
    for (size_t i = 0; i < list_[level].size(); ++ i)
      metas.push_back(list_[level][i]);
    list_[level].clear();
  }
  // select overlap files
  Meta meta = metas[0];
  for (size_t i = 1; i < metas.size(); ++ i) {
    if (metas[i].smallest_.compare(meta.smallest_) < 0)
      meta.smallest_ = Slice(metas[i].smallest_.data(), metas[i].smallest_.size());
    if (metas[i].largest_.compare(meta.largest_) > 0)
      meta.largest_ = Slice(metas[i].largest_.data(), metas[i].largest_.size());
  }
  if (algo == std::string("BiLSMTree")) {
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
  }
  meta = metas[0];
  for (size_t i = 1; i < metas.size(); ++ i) {
    if (metas[i].smallest_.compare(meta.smallest_) < 0)
      meta.smallest_ = Slice(metas[i].smallest_.data(), metas[i].smallest_.size());
    if (metas[i].largest_.compare(meta.largest_) > 0)
      meta.largest_ = Slice(metas[i].largest_.data(), metas[i].largest_.size());
  }
  // select from file_
  for (size_t i = 0; i < file_[level + 1].size(); ) {
    if (file_[level + 1][i].Intersect(meta)) {
      metas.push_back(file_[level + 1][i]);
      file_[level + 1].erase(file_[level + 1].begin() + i);
    }
    else {
      ++ i;
    }
  }
  if (Config::TRACE_LOG)
    std::cout << "Create TableIterator In MajorCompaction Size:" << metas.size() << std::endl;
  std::vector<TableIterator> tables_;
  for (size_t i = 0; i < metas.size(); ++ i) {
    std::string filename = GetFilename(metas[i].sequence_number_);
    // std::cout << "MERGE Filename:" << filename << " from Level:" << metas[i].level_ << "[" << metas[i].smallest_.ToString() << "\t" << metas[i].largest_.ToString() << "]" << std::endl;
    TableIterator t = TableIterator(filename, filesystem_, metas[i], lsmtreeresult_);
    tables_.push_back(t);
    filesystem_->Delete(filename);
  }
  if (Config::TRACE_LOG)
    std::cout << "Create TableIterator In MajorCompaction Success" << std::endl;
  std::vector<Table*> merged_tables = MergeTables(tables_);
  // std::cout << "After Merge Size:" << merged_tables.size() << std::endl;
  for (size_t i = 0; i < merged_tables.size(); ++ i) {
    size_t sequence_number_ = GetSequenceNumber();
    std::string filename = GetFilename(sequence_number_);
    merged_tables[i]->DumpToFile(filename, lsmtreeresult_);
    Meta meta = merged_tables[i]->GetMeta();
    meta.sequence_number_ = sequence_number_;
    meta.level_ = level + 1;
    // std::cout << "In File Level:" << meta.level_ << "\tTable:[" << meta.smallest_.ToString() << "\t" << meta.largest_.ToString() << "]" << std::endl;
    // std::cout << "File DUMP To Level " << level + 1 << std::endl;
    // file_[level + 1].push_back(meta);
    bool add = false;
    for (size_t j = 0; j < file_[level + 1].size(); ++ j) {
      if (meta.largest_.compare(file_[level + 1][j].smallest_) < 0) {
        file_[level + 1].insert(file_[level + 1].begin() + j, meta);
        add = true;
        break;
      }
    }
    if (!add) {
      file_[level + 1].insert(file_[level + 1].end(), meta);
    }
    delete merged_tables[i];
  }
  assert(CheckFileList(level + 1));
  // for (size_t i = 0; i < file_[level + 1].size(); ++ i)
  //   file_[level + 1][i].Show();
  // std::cout << level + 1 << " Sort " << file_[level + 1].size() << std::endl;
  // std::sort(file_[level + 1].begin(), file_[level + 1].end());
  // std::cout << "Sort End" << std::endl;
  lsmtreeresult_->MajorCompaction();
  if (Config::TRACE_LOG)
    std::cout << "MajorCompaction Success" << std::endl;
  size_t level_max_file_Lj = static_cast<size_t>(pow(10, level + 1));
  if (file_[level + 1].size() > level_max_file_Lj)
    MajorCompaction(level + 1);
}

bool LSMTree::CheckFileList(size_t level) {
  if (level == 0) return true;
  for (size_t i = 0; i < file_[level].size(); ++ i) {
    if (file_[level][i].smallest_.compare(file_[level][i].largest_) > 0) {
      std::cout << "META ERROR" << std::endl;
      return false;
    }
    if (i < file_[level].size() - 1 && file_[level][i].largest_.compare(file_[level][i + 1].smallest_) >= 0) {
      std::cout << file_[level][i].largest_.ToString() << "\t" << file_[level][i + 1].smallest_.ToString() << std::endl;
      ShowFileList(level, false);
      std::cout << "FILE LIST ERROR" << std::endl;
      return false;
    }
  }
  return true;
}

void LSMTree::ShowFileList(size_t level, bool show_list) {
  std::cout << std::string(30, '%') << std::endl;
  std::cout << "ShowFileList" << std::endl;
  for (size_t i = 0; i < file_[level].size(); ++ i)
    std::cout << file_[level][i].smallest_.ToString() << "\t" << file_[level][i].largest_.ToString() << std::endl;
  if (show_list) {
    std::cout << "ShowList" << std::endl;
    for (size_t i = 0; i < list_[level].size(); ++ i)
      std::cout << list_[level][i].smallest_.ToString() << "\t" << list_[level][i].largest_.ToString() << std::endl;    
  }
  std::cout << std::string(30, '%') << std::endl;
}

}