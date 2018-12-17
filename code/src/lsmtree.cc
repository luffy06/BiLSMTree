#include "lsmtree.h"

namespace bilsmtree {

LSMTree::LSMTree(FileSystem* filesystem, LSMTreeResult* lsmtreeresult) {
  filesystem_ = filesystem;
  lsmtreeresult_ = lsmtreeresult;
  total_sequence_number_ = 0;
  recent_files_ = new VisitFrequency(Config::VisitFrequencyConfig::MAXQUEUESIZE, filesystem);
  max_size_.resize(Config::LSMTreeConfig::MAX_LEVEL);
  min_size_.resize(Config::LSMTreeConfig::MAX_LEVEL);
  max_size_[0] = Config::LSMTreeConfig::L0SIZE;
  min_size_[0] = Config::LSMTreeConfig::L0SIZE;
  for (size_t i = 1; i < Config::LSMTreeConfig::MAX_LEVEL; ++ i) {
    max_size_[i] = static_cast<size_t>(pow(Config::LSMTreeConfig::LIBASE, i));
    min_size_[i] = static_cast<size_t>(pow(Config::LSMTreeConfig::LIBASE, i));
  }
}

LSMTree::~LSMTree() {
  delete recent_files_;
}

bool LSMTree::Get(const Slice key, Slice& value) {
  // std::cout << "LSMTree Get:" << key.ToString() << std::endl;
  // for (size_t i = 0; i < Config::LSMTreeConfig::MAX_LEVEL; ++ i) {
  //   std::cout << "Level:" << i << " Size:" << max_size_[i] << std::endl;
  //   std::cout << "File Size:" << file_[i].size() << std::endl;
  //   std::cout << "Buffer Size:" << buffer_[i].size() << std::endl;
  //   ShowMetas(i, true);
  // }
  std::string algo = Util::GetAlgorithm();
  size_t checked = 0;
  for (size_t i = 0; i < Config::LSMTreeConfig::MAX_LEVEL; ++ i) {
    std::vector<size_t> check_files_ = GetCheckFiles(algo, i, key);
    if (algo == std::string("BiLSMTree") || algo == std::string("BiLSMTree2")) {
      for (size_t j = 0; j < buffer_[i].size(); ++ j)
        if (key.compare(buffer_[i][j].smallest_) >= 0 && key.compare(buffer_[i][j].largest_) <= 0)
          check_files_.push_back(j + file_[i].size());
    }
    if (Config::TRACE_READ_LOG)
      std::cout << "Level:" << i << " Check Files Size:" << check_files_.size() << std::endl;
    for (size_t j = 0; j < check_files_.size(); ++ j) {
      size_t p = check_files_[j];
      checked = checked + 1;
      Meta meta = (p < file_[i].size() ? file_[i][p] : buffer_[i][p - file_[i].size()]);
      if (Config::TRACE_READ_LOG)
        std::cout << "Check SEQ:" << meta.sequence_number_ << "[" << meta.smallest_.ToString() << ",\t" << meta.largest_.ToString() << "]" << std::endl;
      if (GetValueFromFile(meta, key, value)) {
        if (algo == std::string("BiLSMTree") || algo == std::string("BiLSMTree2")) {
          int deleted = recent_files_->Append(meta.sequence_number_);
          frequency_[meta.sequence_number_] ++;
          if (deleted != -1) frequency_[deleted] --;
          if (i > 0) {
            size_t min_fre = frequency_[file_[i - 1][0].sequence_number_];
            for (size_t k = 1; k < file_[i - 1].size(); ++ k) {
              if (frequency_[file_[i - 1][k].sequence_number_] < min_fre)
                min_fre = frequency_[file_[i - 1][k].sequence_number_];
            }
            if (frequency_[meta.sequence_number_] >= min_fre * (1. + Config::LSMTreeConfig::ALPHA)) {
              if (p < file_[i].size())
                file_[i].erase(file_[i].begin() + p);
              else
                buffer_[i].erase(buffer_[i].begin() + p - file_[i].size());
              RollBack(i, meta);
            }
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
  size_t sequence_number_ = GetSequenceNumber();
  std::string filename = GetFilename(sequence_number_);
  Table *table_ = new Table(kvs, sequence_number_, filename, filesystem_, lsmtreeresult_);
  Meta meta = table_->GetMeta();
  meta.level_ = 0;
  delete table_;

  if (Config::TRACE_LOG)
    std::cout << "DUMP L0:" << filename << "\tRange:[" << meta.smallest_.ToString() << "\t" << meta.largest_.ToString() << "]" << std::endl;
  lsmtreeresult_->MinorCompaction(meta.file_size_);
  std::string algo = Util::GetAlgorithm();
  if (algo == std::string("BiLSMTree") || algo == std::string("BiLSMTree2")) {
    buffer_[0].insert(buffer_[0].begin(), meta);
    int deleted = recent_files_->Append(sequence_number_);
    frequency_[sequence_number_] ++;
    if (deleted != -1) frequency_[deleted] --;
    if (buffer_[0].size() > min_size_[0])
      CompactList(0);
  }
  else {
    file_[0].insert(file_[0].begin(), meta);
    if (file_[0].size() > max_size_[0])
      MajorCompaction(0);
  }
  if (Config::TRACE_LOG)
    std::cout << "DUMP Success" << std::endl;
}

size_t LSMTree::GetSequenceNumber() {
  frequency_.push_back(1);
  return total_sequence_number_ ++;
}

std::string LSMTree::GetFilename(size_t sequence_number_) {
  char filename[100];
  sprintf(filename, "../logs/sstables/sstable_%zu.bdb", sequence_number_);
  return std::string(filename);
}

int LSMTree::BinarySearch(size_t level, const Slice key) {
  if (file_[level].size() == 0) 
    return -1;
  size_t l = 0;
  size_t r = file_[level].size() - 1;
  if (key.compare(file_[level][r].largest_) <= 0 && key.compare(file_[level][l].smallest_) >= 0) {
    if (key.compare(file_[level][l].largest_) <= 0) {
      r = l;
    }
    else {
      // (l, r] <= largest_
      while (r - l > 1) {
        size_t m = (l + r) / 2;
        if (key.compare(file_[level][m].largest_) <= 0)
          r = m;
        else
          l = m;
      }
    }
    if (key.compare(file_[level][r].smallest_) >= 0)
      return r;
  }
  return -1;
}

std::vector<size_t> LSMTree::GetCheckFiles(std::string algo, size_t level, const Slice key) {
  std::vector<size_t> check_files_;
  if (level == 0) {
    if (algo == std::string("BiLSMTree") || algo == std::string("BiLSMTree2")) {
      int index = BinarySearch(level, key);
      if (index != -1)
        check_files_.push_back(index);
    }
    else {
      for (size_t j = 0; j < file_[level].size(); ++ j)
        if (key.compare(file_[level][j].smallest_) >= 0 && key.compare(file_[level][j].largest_) <= 0)
          check_files_.push_back(j);
    }
  }
  else {
    int index = BinarySearch(level, key);
    if (index != -1)
      check_files_.push_back(index);
  }
  return check_files_;
}

void LSMTree::GetOverlaps(std::vector<Meta>& src, std::vector<Meta>& des) {
  if (des.size() == 0)
    return ;
  // get overlap range
  Meta meta = des[0];
  for (size_t i = 1; i < des.size(); ++ i) {
    if (des[i].smallest_.compare(meta.smallest_) < 0)
      meta.smallest_ = des[i].smallest_;
    if (des[i].largest_.compare(meta.largest_) > 0)
      meta.largest_ = des[i].largest_;
  }
  size_t len = des.size();
  for (size_t i = 0; i < src.size(); ) {
    if (src[i].Intersect(meta)) {
      des.push_back(src[i]);
      src.erase(src.begin() + i);
    }
    else {
      ++ i;
    }
  } 
}

bool LSMTree::GetValueFromFile(const Meta meta, const Slice key, Slice& value) {
  if (Config::TRACE_READ_LOG) {
    std::cout << "Get Value From File Key:" << key.ToString() << std::endl;
    std::cout << "SEQ:" << meta.sequence_number_ << " [" << meta.smallest_.ToString() << ",\t" << meta.largest_.ToString() << "]" << std::endl;
  }
  std::stringstream ss;
  std::string filename = GetFilename(meta.sequence_number_);
  
  if (Config::TRACE_READ_LOG)
    std::cout << "Read Footer Block" << std::endl;
  filesystem_->Open(filename, Config::FileSystemConfig::READ_OPTION);
  filesystem_->Seek(filename, meta.file_size_ - meta.footer_size_);
  std::string offset_data_ = filesystem_->Read(filename, meta.footer_size_);
  ss.str(offset_data_);
  lsmtreeresult_->Read(offset_data_.size());
  size_t index_offset_ = 0;
  size_t filter_offset_ = 0;
  ss >> index_offset_;
  ss >> filter_offset_;
  assert(index_offset_ != 0);
  assert(filter_offset_ != 0);
  if (Config::TRACE_READ_LOG)
    std::cout << "Read Footer Block Success! Index Offset:" << index_offset_ << " Filter Offset:" << filter_offset_ << std::endl;

  // READ FILTER
  if (Config::TRACE_READ_LOG)
    std::cout << "Read Filter Block" << std::endl;
  filesystem_->Seek(filename, filter_offset_);
  std::string filter_data_ = filesystem_->Read(filename, meta.file_size_ - filter_offset_ - meta.footer_size_);
  lsmtreeresult_->Read(filter_data_.size());
  Filter* filter = NULL;
  std::string algo = Util::GetAlgorithm();
  if (Config::TRACE_READ_LOG)
    std::cout << "Create Filter" << std::endl;
  if (algo == std::string("LevelDB") || algo == std::string("Wisckey")) {
    filter = new BloomFilter(filter_data_);
  }
  else if (algo == std::string("BiLSMTree") || algo == std::string("BiLSMTree2")) {
    filter = new CuckooFilter(filter_data_);
  }
  else {
    filter = NULL;
    assert(false);
  }
  if (Config::TRACE_READ_LOG)
    std::cout << "Filter Block KeyMatch" << std::endl;
  if (!filter->KeyMatch(key)) {
    filesystem_->Close(filename);
    if (Config::TRACE_READ_LOG)
      std::cout << "Filter Doesn't Exist" << std::endl;
    delete filter;
    return false;
  }
  delete filter;
  if (Config::TRACE_READ_LOG)
    std::cout << "Read Filter Block Success" << std::endl;

  // READ INDEX BLOCK
  if (Config::TRACE_READ_LOG)
    std::cout << "Read Index Block" << std::endl;
  filesystem_->Seek(filename, index_offset_);
  std::string index_data_ = filesystem_->Read(filename, filter_offset_ - index_offset_);
  lsmtreeresult_->Read(index_data_.size());
  ss.str(index_data_);
  bool found = false;
  size_t offset_ = 0;
  size_t data_block_size_ = 0;
  while (true) {
    std::string largest_str;
    ss >> largest_str >> offset_ >> data_block_size_;
    Slice largest_ = Slice(largest_str.data(), largest_str.size());

    if (ss.tellg() == -1) 
      break;
    if (key.compare(largest_) <= 0) {
      found = true;
      break;
    }
  }
  if (!found) {
    if (Config::TRACE_READ_LOG)
      std::cout << "Index Block Doesn't Exist" << std::endl;
    filesystem_->Close(filename);
    return false;
  }
  if (Config::TRACE_READ_LOG)
    std::cout << "Read Index Block Success" << std::endl;

  // READ DATA BLOCK
  if (Config::TRACE_READ_LOG)
    std::cout << "Read Data Block" << std::endl;
  filesystem_->Seek(filename, offset_);
  std::string data_ = filesystem_->Read(filename, data_block_size_);
  lsmtreeresult_->Read(data_.size());
  filesystem_->Close(filename);
  ss.str(data_);
  while (true) {
    std::string key_str, value_str;
    ss >> key_str >> value_str;
    Slice key_ = Slice(key_str.data(), key_str.size());
    Slice value_ = Slice(value_str.data(), value_str.size());

    if (ss.tellg() == -1)
      break;
    if (key.compare(key_) == 0) {
    if (Config::TRACE_READ_LOG)
      std::cout << "Find Key" << std::endl;
      value = value_;
      return true;
    }
  }
  if (Config::TRACE_READ_LOG)
    std::cout << "Read Data Block Success! Key Doesn't Exist" << std::endl;  
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

void LSMTree::RollBack(const size_t now_level, const Meta meta) {
  std::string algo = Util::GetAlgorithm();
  assert(algo == std::string("BiLSMTree") || algo == std::string("BiLSMTree2"));
  if (now_level == 0)
    return ;
  size_t to_level = GetTargetLevel(now_level, meta);
  assert(to_level <= now_level);
  if (to_level == now_level)
    return ;
  if (Config::TRACE_LOG) {
    std::cout << "RollBack Now Level:" << now_level << " To Level:" << to_level << std::endl;
    std::cout << "SEQ:" << meta.sequence_number_ << " [" << meta.smallest_.ToString() << ",\t" << meta.largest_.ToString() << "]" << std::endl;
  }
  std::stringstream ss;
  std::string filename = GetFilename(meta.sequence_number_);
  filesystem_->Open(filename, Config::FileSystemConfig::READ_OPTION | Config::FileSystemConfig::WRITE_OPTION);
  filesystem_->Seek(filename, meta.file_size_ - meta.footer_size_);
  std::string footer_data_ = filesystem_->Read(filename, meta.footer_size_);
  lsmtreeresult_->Read(footer_data_.size());

  ss.str(footer_data_);
  size_t index_offset_ = 0;
  size_t filter_offset_ = 0;
  ss >> index_offset_;
  ss >> filter_offset_;
  assert(index_offset_ != 0);
  assert(filter_offset_ != 0);

  // MEGER FILTER
  filesystem_->Seek(filename, filter_offset_);
  std::string filter_data_ = filesystem_->Read(filename, meta.file_size_ - filter_offset_ - meta.footer_size_);
  lsmtreeresult_->Read(filter_data_.size());
  CuckooFilter *filter = new CuckooFilter(filter_data_);
  if (Config::TRACE_LOG)
    std::cout << "Get Complement Filter" << std::endl;
  for (int i = now_level - 1; i >= static_cast<int>(to_level); -- i) {
    int l = 0;
    int r = static_cast<int>(file_[i].size()) - 1;
    while (l < static_cast<int>(file_[i].size()) && meta.smallest_.compare(file_[i][l].largest_) > 0) ++ l;
    while (r >= 0 && meta.largest_.compare(file_[i][r].smallest_) < 0) -- r;
    if (l > r)
      continue;

    for (int j = l; j <= r; ++ j) {
      Meta to_meta = file_[i][j];
      std::string to_filename = GetFilename(to_meta.sequence_number_);
      filesystem_->Open(to_filename, Config::FileSystemConfig::READ_OPTION);
      filesystem_->Seek(to_filename, to_meta.file_size_ - to_meta.footer_size_);
      std::string to_offset_data_ = filesystem_->Read(to_filename, to_meta.footer_size_);
      lsmtreeresult_->Read(to_offset_data_.size());

      ss.str(to_offset_data_);
      size_t to_index_offset_ = 0;
      size_t to_filter_offset_ = 0;
      ss >> to_index_offset_;
      ss >> to_filter_offset_;
      assert(to_index_offset_ != 0);
      assert(to_filter_offset_ != 0);

      filesystem_->Seek(to_filename, to_filter_offset_);
      std::string to_filter_data_ = filesystem_->Read(to_filename, to_meta.file_size_ - to_filter_offset_ - to_meta.footer_size_);
      lsmtreeresult_->Read(to_filter_data_.size());
      CuckooFilter *to_filter = new CuckooFilter(to_filter_data_);

      filter->Diff(to_filter);
      delete to_filter;
      filesystem_->Close(to_filename);
    }
  }
  if (Config::TRACE_LOG)
    std::cout << "Get Complement Filter Success" << std::endl;
  
  filesystem_->Seek(filename, filter_offset_);
  int file_size_minus_ = filter_data_.size() - filter->ToString().size();
  assert(file_size_minus_ >= 0);
  filter_data_ = filter->ToString();
  filesystem_->Write(filename, filter_data_.data(), filter_data_.size());
  filesystem_->Write(filename, footer_data_.data(), footer_data_.size());
  filesystem_->SetFileSize(filename, meta.file_size_ - file_size_minus_);
  lsmtreeresult_->Write(filter_data_.size() + footer_data_.size());  
  delete filter;
  filesystem_->Close(filename);

  Meta new_meta;
  new_meta.Copy(meta);
  new_meta.file_size_ = meta.file_size_ - file_size_minus_;
  new_meta.level_ = to_level;

  // add to buffer_
  buffer_[to_level].push_back(new_meta);
  if (buffer_[to_level].size() > Config::LSMTreeConfig::LISTSIZE) {
    CompactList(to_level);
    max_size_[to_level] = max_size_[to_level] + Config::LSMTreeConfig::LISTSIZE;
  }
  if (Config::TRACE_LOG)
    std::cout << "RollBack Success" << std::endl;
}

std::vector<Table*> LSMTree::MergeTables(const std::vector<TableIterator>& tables) {
  if (Config::TRACE_LOG)
    std::cout << "MergeTables Start:" << tables.size() << std::endl;
  
  std::vector<KV> buffers_;
  size_t buffer_size_ = 0;
  std::vector<Table*> result_;
  std::priority_queue<TableIterator> q;
  
  for (size_t i = 0; i < tables.size(); ++ i) {
    TableIterator ti = tables[i];
    ti.SetId(i);
    // std::cout << "Ready Merge Table " << i << " Size:" << ti.DataSize() << std::endl;
    if (ti.HasNext())
      q.push(ti);
  }

  size_t table_size_ = Util::GetSSTableSize();

  while (!q.empty()) {
    TableIterator ti = q.top();
    q.pop();
    KV kv = ti.Next();
    if (ti.HasNext())
      q.push(ti);
    if (buffers_.size() == 0 || kv.key_.compare(buffers_[buffers_.size() - 1].key_) > 0) {
      if (buffer_size_ >= table_size_) {
        size_t sequence_number_ = GetSequenceNumber();
        std::string filename = GetFilename(sequence_number_);
        Table *t = new Table(buffers_, sequence_number_, filename, filesystem_, lsmtreeresult_);
        result_.push_back(t);
        buffers_.clear();
        buffer_size_ = 0;
      }
      buffers_.push_back(kv);
      buffer_size_ = buffer_size_ + kv.size();
    }
  }
  if (buffer_size_ > 0) {
    size_t sequence_number_ = GetSequenceNumber();
    std::string filename = GetFilename(sequence_number_);
    Table *t = new Table(buffers_, sequence_number_, filename, filesystem_, lsmtreeresult_);
    result_.push_back(t);
  }
  if (Config::TRACE_LOG)
    std::cout << "MergeTables End:" << result_.size() << std::endl;
  return result_;
}

void LSMTree::CompactList(size_t level) {
  if (Config::TRACE_LOG)
    std::cout << "CompactList On Level:" << level << std::endl;
  std::vector<Meta> wait_queue_;
  for (size_t i = 0; i < buffer_[level].size(); ++ i)
    wait_queue_.push_back(buffer_[level][i]);
  buffer_[level].clear();
  GetOverlaps(file_[level], wait_queue_);
  std::vector<TableIterator> tables_;
  for (size_t i = 0; i < wait_queue_.size(); ++ i) {
    std::string filename = GetFilename(wait_queue_[i].sequence_number_);
    tables_.push_back(TableIterator(filename, filesystem_, wait_queue_[i], lsmtreeresult_));
    filesystem_->Delete(filename);
  }
  std::vector<Table*> merged_tables = MergeTables(tables_);
  for (size_t i = 0; i < merged_tables.size(); ++ i) {
    Meta meta = merged_tables[i]->GetMeta();
    delete merged_tables[i];
    meta.level_ = level;
    bool inserted = false;
    for (size_t j = 0; j < file_[level].size(); ++ j) {
      if (meta.largest_.compare(file_[level][j].smallest_) <= 0) {
        file_[level].insert(file_[level].begin() + j, meta);
        inserted = true;
        break;
      }
    }
    if (!inserted)
      file_[level].insert(file_[level].end(), meta);
  }

  if (file_[level].size() > max_size_[level])
    MajorCompaction(level);
  if (Config::TRACE_LOG)
    std::cout << "CompactList On Level:" << level << " Success" << std::endl;
}

void LSMTree::MajorCompaction(size_t level) {
  std::string algo = Util::GetAlgorithm();
  if (level == Config::LSMTreeConfig::MAX_LEVEL)
    return ;
  std::vector<Meta> wait_queue_;
  if (file_[level].size() <= max_size_[level])
    return ;
  if (Config::TRACE_LOG)
    std::cout << "MajorCompaction On Level:" << level << std::endl;
  size_t select_numb_Li = file_[level].size() - max_size_[level];
  // std::cout << "SELECT " << select_numb_Li << std::endl;
  max_size_[level] = std::max(max_size_[level] - 1, min_size_[level]);
  // select from files from Li
  std::vector<std::pair<size_t, size_t>> indexs;
  std::vector<size_t> select_indexs;
  for (size_t i = 0; i < file_[level].size(); ++ i)
    indexs.push_back(std::make_pair(i, frequency_[file_[level][i].sequence_number_]));
  std::sort(indexs.begin(), indexs.end(), [](std::pair<size_t, size_t> a, std::pair<size_t, size_t> b) {
    if (a.second != b.second)
      return a.second < b.second;
    return a.first < b.first;
  });
  for (size_t i = 0; i < select_numb_Li; ++ i)
    select_indexs.push_back(indexs[i].first);
  sort(select_indexs.begin(), select_indexs.end(), [](size_t a, size_t b) {
    return a < b;
  });
  for (size_t i = 0; i < select_indexs.size(); ++ i) {
    size_t p = select_indexs[i];
    wait_queue_.push_back(file_[level][p - i]);
    file_[level].erase(file_[level].begin() + p - i);
  }

  // select overlap files from Li+1
  if (algo == std::string("BiLSMTree") || algo == std::string("BiLSMTree2"))
    GetOverlaps(buffer_[level + 1], wait_queue_);
  // select from file_
  GetOverlaps(file_[level + 1], wait_queue_);

  std::vector<TableIterator> tables_;
  size_t total_size_ = 0;
  for (size_t i = 0; i < wait_queue_.size(); ++ i) {
    std::string filename = GetFilename(wait_queue_[i].sequence_number_);
    total_size_ = total_size_ + wait_queue_[i].file_size_;
    tables_.push_back(TableIterator(filename, filesystem_, wait_queue_[i], lsmtreeresult_));
    filesystem_->Delete(filename);
  }
  lsmtreeresult_->MajorCompaction(total_size_);
  std::vector<Table*> merged_tables = MergeTables(tables_);
  for (size_t i = 0; i < merged_tables.size(); ++ i) {
    Meta meta = merged_tables[i]->GetMeta();
    delete merged_tables[i];
    meta.level_ = level + 1;
    
    bool inserted = false;
    for (size_t j = 0; j < file_[level + 1].size(); ++ j) {
      if (meta.largest_.compare(file_[level + 1][j].smallest_) <= 0) {
        file_[level + 1].insert(file_[level + 1].begin() + j, meta);
        inserted = true;
        break;
      }
    }
    if (!inserted) {
      file_[level + 1].insert(file_[level + 1].end(), meta);
    }
  }
  assert(CheckFileList(level + 1));
  if (Config::TRACE_LOG)
    std::cout << "MajorCompaction On Level:" << level << " Success" << std::endl;
  if (file_[level + 1].size() > max_size_[level + 1])
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
      ShowMetas(level, false);
      std::cout << "FILE LIST ERROR" << std::endl;
      return false;
    }
  }
  return true;
}

void LSMTree::ShowMetas(size_t level, bool show_buffer) {
  std::cout << std::string(30, '%') << std::endl;
  std::cout << "File" << std::endl;
  for (size_t i = 0; i < file_[level].size(); ++ i) {
    Meta m = file_[level][i];
    std::cout << "SEQ:" << m.sequence_number_ << " [" << m.smallest_.ToString() << ",\t" << m.largest_.ToString() << "]" << std::endl;
  }
  if (show_buffer) {
    std::cout << "Buffer" << std::endl;
    for (size_t i = 0; i < buffer_[level].size(); ++ i) {
      Meta m = buffer_[level][i];
      std::cout << "SEQ:" << m.sequence_number_ << " [" << m.smallest_.ToString() << ",\t" << m.largest_.ToString() << "]" << std::endl;
    }
  }
  std::cout << std::string(30, '%') << std::endl;
}

}