#include "lsmtree.h"

namespace bilsmtree {

LSMTree::LSMTree(FileSystem* filesystem, LSMTreeResult* lsmtreeresult) {
  filesystem_ = filesystem;
  lsmtreeresult_ = lsmtreeresult;
  total_sequence_number_ = 0;
  rollback_ = 0;
  ALPHA = 5.0;
  recent_files_ = new VisitFrequency(Config::VisitFrequencyConfig::MAXQUEUESIZE, filesystem);
  datamanager_ = new DataManager(filesystem);
  filtermanager_ = new FilterManager(filesystem);

  std::string algo = Util::GetAlgorithm();
  max_size_.resize(Config::LSMTreeConfig::MAX_LEVEL);
  min_size_.resize(Config::LSMTreeConfig::MAX_LEVEL);
  buf_size_.resize(Config::LSMTreeConfig::MAX_LEVEL);
  max_size_[0] = Config::LSMTreeConfig::L0SIZE;
  min_size_[0] = Config::LSMTreeConfig::L0SIZE;
  buf_size_[0] = Config::LSMTreeConfig::L0SIZE;
  for (size_t i = 1; i < Config::LSMTreeConfig::MAX_LEVEL; ++ i) {
    max_size_[i] = static_cast<size_t>(pow(Config::LSMTreeConfig::LIBASE, i));
    min_size_[i] = static_cast<size_t>(pow(Config::LSMTreeConfig::LIBASE, i));
    buf_size_[i] = 0;
    if (algo == std::string("BiLSMTree") || algo == std::string("BiLSMTree2"))
      buf_size_[i] = max_size_[i] / 2 < Config::LSMTreeConfig::LISTSIZE ? max_size_[i] / 2 : Config::LSMTreeConfig::LISTSIZE;
    max_size_[i] = max_size_[i] - buf_size_[i];
  }
}

LSMTree::~LSMTree() {
  delete recent_files_;
  delete datamanager_;
  delete filtermanager_;
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
          UpdateFrequency(meta.sequence_number_);
          if (i > 0) {
            size_t min_fre = frequency_[file_[i - 1][0].sequence_number_];
            for (size_t k = 1; k < file_[i - 1].size(); ++ k) {
              if (frequency_[file_[i - 1][k].sequence_number_] < min_fre)
                min_fre = frequency_[file_[i - 1][k].sequence_number_];
            }
            if (frequency_[meta.sequence_number_] >= min_fre * ALPHA) {
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
  size_t total_size_ = 0;
  for (size_t i = 0; i < kvs.size(); ++ i)
    total_size_ = total_size_ + kvs[i].size();
  size_t sequence_number_ = GetSequenceNumber();
  std::string algo = Util::GetAlgorithm();
  std::string filename_ = GetFilename(sequence_number_);
  Table *table_ = NULL;
  if (algo == std::string("LevelDB-Sep")) {
    std::vector<BlockMeta> bms = datamanager_->Append(kvs);
    table_ = new IndexTable(bms, sequence_number_, filename_, filesystem_, lsmtreeresult_);
  }
  else {
    table_ = new Table(kvs, sequence_number_, filename_, filesystem_, filtermanager_, lsmtreeresult_);
  }
  UpdateFrequency(sequence_number_);
  Meta meta = table_->GetMeta();
  meta.level_ = 0;
  delete table_;

  if (Config::TRACE_LOG)
    std::cout << "DUMP L0:" << filename_ << "\tRange:[" << meta.smallest_.ToString() << "\t" << meta.largest_.ToString() << "]" << std::endl;
  lsmtreeresult_->MinorCompaction(total_size_);
  std::string algo = Util::GetAlgorithm();
  if (algo == std::string("BiLSMTree") || algo == std::string("BiLSMTree2")) {
    buffer_[0].insert(buffer_[0].begin(), meta);
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

std::string LSMTree::GetFilename(size_t sequence_number_) {
  char filename[100];
  sprintf(filename, "../logs/sstables/sstable_%zu.bdb", sequence_number_);
  return std::string(filename);
}

void LSMTree::UpdateFrequency(size_t sequence_number) {
  int deleted = recent_files_->Append(sequence_number);
  frequency_[sequence_number] ++;
  if (deleted != -1) frequency_[deleted] --;
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

std::string LSMTree::GetFilterData(const std::string filter_data) {
  // std::stringstream ss(filter_data);
  // size_t offset_ = 0;
  // size_t filter_size_ = 0;
  // ss >> offset_ >> filter_size_;
  // filter_data = filtermanager_->Get(offset_, filter_size_);
  // lsmtreeresult_->Read(filter_size_, "FILTER");
  return filter_data;
}

std::string LSMTree::WriteFilterData(const std::string filter_data) {
  // std::pair<size_t, size_t> loc = filtermanager_->Append(filter_data);
  // std::stringstream ss;
  // ss << loc.first << Config::DATA_SEG << loc.second << Config::DATA_SEG;
  // filter_data = ss.str();
  return filter_data;
}

bool LSMTree::GetValueFromFile(const Meta meta, const Slice key, Slice& value) {
  if (Config::TRACE_READ_LOG) {
    std::cout << "Get Value From File Key:" << key.ToString() << std::endl;
    std::cout << "SEQ:" << meta.sequence_number_ << " [" << meta.smallest_.ToString() << ",\t" << meta.largest_.ToString() << "]" << std::endl;
  }
  std::string algo = Util::GetAlgorithm();
  std::stringstream ss;
  std::string filename = GetFilename(meta.sequence_number_);
  
  if (Config::TRACE_READ_LOG)
    std::cout << "Read Footer Block" << std::endl;
  filesystem_->Open(filename, Config::FileSystemConfig::READ_OPTION);
  filesystem_->Seek(filename, meta.file_size_ - meta.footer_size_);
  std::string offset_data_ = filesystem_->Read(filename, meta.footer_size_);
  lsmtreeresult_->Read(offset_data_.size(), "FOOTER");
  ss.str(offset_data_);
  size_t index_offset_ = 0;
  size_t filter_offset_ = 0;
  ss >> index_offset_;
  ss >> filter_offset_;
  if (algo != std::string("LevelDB-Sep"))
    assert(index_offset_ != 0);
  assert(filter_offset_ != 0);
  if (Config::TRACE_READ_LOG)
    std::cout << "Read Footer Block Success! Index Offset:" << index_offset_ << " Filter Offset:" << filter_offset_ << std::endl;

  // READ FILTER
  if (Config::TRACE_READ_LOG)
    std::cout << "Read Filter Block" << std::endl;
  filesystem_->Seek(filename, filter_offset_);
  std::string filter_data_ = filesystem_->Read(filename, meta.file_size_ - filter_offset_ - meta.footer_size_);
  lsmtreeresult_->Read(filter_data_.size(), "FILTER");
  std::vector<Filter*> filters;
  if (Config::TRACE_READ_LOG)
    std::cout << "Create Filter" << std::endl;
  if (algo == std::string("LevelDB") || algo == std::string("Wisckey")) {
    filters.push_back(new BloomFilter(filter_data_));
  }
  else if (algo == std::string("BiLSMTree")) {
    filters.push_back(new CuckooFilter(filter_data_));
  }
  else if (algo == std::string("LevelDB-Sep")) {
    ss.str(filter_data_);
    size_t numb_filter_;
    ss >> numb_filter_;
    for (size_t i = 0; i < numb_filter_; ++ i) {
      size_t size_;
      ss >> size_;
      char *filter_str = new char[size_ + 1];
      ss.read(filter_str, sizeof(char) * size_);
      filter_str[size_] = '\0';
      filter_data_ = std::string(filter_str, size_);
      delete[] filter_str;
      filters.push_back(new BloomFilter(filter_data_));
    }
  }
  if (Config::TRACE_READ_LOG)
    std::cout << "Filter Block KeyMatch" << std::endl;
  bool keymatch = false;
  for (size_t i = 0; i < filters.size(); ++ i) {
    if (filters[i]->KeyMatch(key))
      keymatch = true;
    delete filters[i];
  }
  if (!keymatch) {
    filesystem_->Close(filename);
    if (Config::TRACE_READ_LOG)
      std::cout << "Filter Doesn't Exist" << std::endl;
    return false;
  }
  if (Config::TRACE_READ_LOG)
    std::cout << "Read Filter Block Success\nRead Index Block" << std::endl;

  // READ INDEX BLOCK
  filesystem_->Seek(filename, index_offset_);
  std::string index_data_ = filesystem_->Read(filename, filter_offset_ - index_offset_);
  lsmtreeresult_->Read(index_data_.size(), "INDEX");
  ss.str(index_data_);
  bool found = false;
  size_t offset_ = 0;
  size_t block_size_ = 0;
  size_t block_numb_ = 0;
  size_t n;
  ss >> n;
  for (size_t i = 0; i < n; ++ i) {
    std::string smallest_str;
    std::string largest_str;
    if (algo != std::string("LevelDB-Sep"))
      ss >> largest_str >> offset_ >> block_size_;
    else
      ss >> smallest_str >> largest_str >> offset_ >> block_size_ >> block_numb_;
    Slice largest_ = Slice(largest_str.data(), largest_str.size());
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
  if (algo == std::string("LevelDB-Sep")) {
    filesystem_->Close(filename);
    if (datamanager_->Get(key, value, offset_, block_size_))
      return true;
  }
  else {
    filesystem_->Seek(filename, offset_);
    std::string data_ = filesystem_->Read(filename, block_size_);
    lsmtreeresult_->Read(data_.size());
    filesystem_->Close(filename);
    ss.str(data_);
    ss >> n;
    for (size_t i = 0; i < n; ++ i) {
      std::string key_str, value_str;
      ss >> key_str >> value_str;
      Slice key_ = Slice(key_str.data(), key_str.size());
      Slice value_ = Slice(value_str.data(), value_str.size());

      if (key.compare(key_) == 0) {
        if (Config::TRACE_READ_LOG)
          std::cout << "Find Key" << std::endl;
        value = value_;
        return true;
      }
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
    double diff_t = frequency_[meta.sequence_number_] * 3 * (now_level - i) * (buf_size_[now_level] + 1) - Config::FlashConfig::READ_WRITE_RATE - overlaps[i] - 1;
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
  lsmtreeresult_->RollBack();
  if (Config::TRACE_LOG) {
    std::cout << "RollBack Now Level:" << now_level << " To Level:" << to_level << std::endl;
    std::cout << "SEQ:" << meta.sequence_number_ << " [" << meta.smallest_.ToString() << ",\t" << meta.largest_.ToString() << "]" << std::endl;
  }
  std::stringstream ss;
  std::string filename = GetFilename(meta.sequence_number_);
  filesystem_->Open(filename, Config::FileSystemConfig::READ_OPTION | Config::FileSystemConfig::WRITE_OPTION);
  filesystem_->Seek(filename, meta.file_size_ - meta.footer_size_);
  std::string footer_data_ = filesystem_->Read(filename, meta.footer_size_);
  lsmtreeresult_->Read(footer_data_.size(), "FOOTER");

  ss.str(footer_data_);
  size_t index_offset_ = 0;
  size_t filter_offset_ = 0;
  ss >> index_offset_;
  ss >> filter_offset_;
  assert(index_offset_ != 0);
  assert(filter_offset_ != 0);

  // MEGER FILTER
  filesystem_->Seek(filename, filter_offset_);
  std::string filter_loc_data_ = filesystem_->Read(filename, meta.file_size_ - filter_offset_ - meta.footer_size_);
  lsmtreeresult_->Read(filter_loc_data_.size(), "FILTER");
  std::string filter_data_ = GetFilterData(filter_loc_data_);
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
      lsmtreeresult_->Read(to_offset_data_.size(), "FOOTER");

      ss.str(to_offset_data_);
      size_t to_index_offset_ = 0;
      size_t to_filter_offset_ = 0;
      ss >> to_index_offset_;
      ss >> to_filter_offset_;
      assert(to_index_offset_ != 0);
      assert(to_filter_offset_ != 0);

      filesystem_->Seek(to_filename, to_filter_offset_);
      std::string to_filter_loc_data_ = filesystem_->Read(to_filename, to_meta.file_size_ - to_filter_offset_ - to_meta.footer_size_);
      lsmtreeresult_->Read(to_filter_loc_data_.size(), "FILTER");
      std::string to_filter_data_ = GetFilterData(to_filter_loc_data_);
      CuckooFilter *to_filter = new CuckooFilter(to_filter_data_);

      filter->Diff(to_filter);
      delete to_filter;
      filesystem_->Close(to_filename);
    }
  }
  if (Config::TRACE_LOG)
    std::cout << "Get Complement Filter Success" << std::endl;
  
  filesystem_->Seek(filename, filter_offset_);
  filter_data_ = filter->ToString();
  std::string new_filter_loc_data_ = WriteFilterData(filter_data_);

  int file_size_change_ = filter_loc_data_.size() - new_filter_loc_data_.size();
  if (Config::WRITE_OUTPUT)
    std::cout << "WRITE FILTER ROLLBACK " << new_filter_loc_data_.size() << std::endl;
  filesystem_->Write(filename, new_filter_loc_data_.data(), new_filter_loc_data_.size());
  filesystem_->Write(filename, footer_data_.data(), footer_data_.size());
  filesystem_->SetFileSize(filename, meta.file_size_ - file_size_change_);
  filesystem_->Close(filename);
  lsmtreeresult_->Write(new_filter_loc_data_.size() + footer_data_.size());
  delete filter;

  Meta new_meta;
  new_meta.Copy(meta);
  new_meta.file_size_ = meta.file_size_ - file_size_change_;
  new_meta.level_ = to_level;

  // add to buffer_
  buffer_[to_level].push_back(new_meta);
  if (buffer_[to_level].size() > buf_size_[to_level]) {
    max_size_[to_level] = max_size_[to_level] + buf_size_[to_level] * 2;
    CompactList(to_level);
  }
  if (Config::TRACE_LOG)
    std::cout << "RollBack Success" << std::endl;
}

std::vector<Table*> LSMTree::MergeTables(const std::vector<TableIterator*>& tables) {
  if (Config::TRACE_LOG)
    std::cout << "MergeTables Start:" << tables.size() << std::endl;
  
  std::vector<KV> buffers_;
  size_t buffer_size_ = 0;
  std::vector<Table*> result_;
  std::priority_queue<TableIterator*, std::vector<TableIterator*>, TableComparator> q;
  
  for (size_t i = 0; i < tables.size(); ++ i) {
    TableIterator *ti = tables[i];
    ti->SetId(i);
    // std::cout << "Ready Merge Table " << i << " Size:" << ti.DataSize() << std::endl;
    if (ti->HasNext())
      q.push(ti);
  }

  size_t table_size_ = Util::GetSSTableSize();
  while (!q.empty()) {
    TableIterator *ti = q.top();
    q.pop();
    KV kv = ti->Next();
    if (ti->HasNext())
      q.push(ti);
    if (buffers_.size() == 0 || kv.key_.compare(buffers_[buffers_.size() - 1].key_) > 0) {
      if (buffer_size_ >= table_size_) {
        size_t sequence_number_ = GetSequenceNumber();
        std::string filename = GetFilename(sequence_number_);
        Table *t = new Table(buffers_, sequence_number_, filename, filesystem_, filtermanager_, lsmtreeresult_);
        UpdateFrequency(sequence_number_);
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
    Table *t = new Table(buffers_, sequence_number_, filename, filesystem_, filtermanager_, lsmtreeresult_);
    UpdateFrequency(sequence_number_);
    result_.push_back(t);
  }
  for (size_t i = 0; i < tables.size(); ++ i)
    delete tables[i];
  if (Config::TRACE_LOG)
    std::cout << "MergeTables End:" << result_.size() << std::endl;
  return result_;
}

std::vector<Table*> LSMTree::MergeIndexTables(const std::vector<IndexTableIterator*>& tables) {
  if (Config::TRACE_LOG)
    std::cout << "MergeIndexTables Start:" << tables.size() << std::endl;
  
  std::vector<KV> kv_buffers_;
  std::vector<BlockMeta> index_buffers_;
  std::vector<Table*> result_;
  std::priority_queue<IndexTableIterator*, std::vector<IndexTableIterator*>, IndexTableComparator> q;
  std::vector<std::pair<size_t, BlockMeta>> overlaps;
  size_t data_size_ = 0;
  bool first = true;
  Slice largest_;
  
  size_t total_blocks_ = 0;
  size_t still_blocks_ = 0;
  for (size_t i = 0; i < tables.size(); ++ i) {
    IndexTableIterator *ti = tables[i];
    ti->SetId(i);
    total_blocks_ = total_blocks_ + ti->DataSize();
    if (ti->HasNext())
      q.push(ti);
  }

  size_t table_size_ = Util::GetSSTableSize();
  while (!q.empty() || overlaps.size() > 0) {
    if (!q.empty()) {
      IndexTableIterator *ti = q.top();
      q.pop();
      BlockMeta bm = ti->Next();
      if (first) {
        largest_ = bm.largest_;
        first = false;
      }
      else if (largest_.compare(bm.largest_) < 0) {
        largest_ = bm.largest_;
      }
      if (ti->HasNext())
        q.push(ti);
      overlaps.push_back(std::make_pair(ti->Id(), bm));
      if (!q.empty() && largest_.compare(q.top()->Current().smallest_) >= 0)
        continue ;
    }
    
    // if (Config::TRACE_LOG)
    //   std::cout << "Merge All Overlaps:" << overlaps.size() << std::endl;
    first = true;
    // merge all overlaped files
    if (overlaps.size() == 1) {
      // if (Config::TRACE_LOG)
      //   std::cout << "Keep Block" << std::endl;
      still_blocks_ = still_blocks_ + 1;
      std::vector<BlockMeta> bms;
      if (kv_buffers_.size() > 0) {
        std::vector<BlockMeta> nbm = datamanager_->Append(kv_buffers_);
        kv_buffers_.clear();
        for (size_t j = 0; j < nbm.size(); ++ j)
          bms.push_back(nbm[j]);
      }
      bms.push_back(overlaps[0].second);
      for (size_t i = 0; i < bms.size(); ++ i) {
        index_buffers_.push_back(bms[i]);
        data_size_ = data_size_ + bms[i].block_size_;
        if (data_size_ >= table_size_) {
          size_t sequence_number_ = GetSequenceNumber();
          std::string filename = GetFilename(sequence_number_);
          Table *it = new IndexTable(index_buffers_, sequence_number_, filename, filesystem_, lsmtreeresult_);
          result_.push_back(it);
          index_buffers_.clear();
          data_size_ = 0;
        }
      }
    }
    else {
      // std::cout << "Merge Blocks" << std::endl;
      std::vector<BlockIterator*> bis;
      for (size_t i = 0; i < overlaps.size(); ++ i) {
        std::string block_data_ = datamanager_->ReadBlock(overlaps[i].second);
        BlockIterator *bi = new BlockIterator(overlaps[i].first, overlaps[i].second, block_data_);
        bis.push_back(bi);
      }
      MergeBlocks(bis, kv_buffers_);

      for (size_t i = 0; i < bis.size(); ++ i)
        delete bis[i];
      for (size_t i = 0; i < overlaps.size(); ++ i)
        datamanager_->Invalidate(overlaps[i].second);
      // std::cout << "Merge Blocks END" << std::endl;
    }
    overlaps.clear();
  }
  if (data_size_ > 0) {
    size_t sequence_number_ = GetSequenceNumber();
    std::string filename = GetFilename(sequence_number_);
    Table *it = new IndexTable(index_buffers_, sequence_number_, filename, filesystem_, lsmtreeresult_);
    result_.push_back(it);
    index_buffers_.clear();
  }
  for (size_t i = 0; i < tables.size(); ++ i)
    delete tables[i];
  lsmtreeresult_->KeepBlock((still_blocks_ * 1.0) / total_blocks_);
  // if (Config::TRACE_LOG)
  //   std::cout << "MergeIndexTables End:" << result_.size() << std::endl;
  return result_;
}

void LSMTree::MergeBlocks(const std::vector<BlockIterator*>& bis, std::vector<KV>& kv_buffers) {
  std::priority_queue<BlockIterator*, std::vector<BlockIterator*>, BlockComparator> q;
  for (size_t i = 0; i < bis.size(); ++ i) {
    if (bis[i]->HasNext())
      q.push(bis[i]);
  }
  while (!q.empty()) {
    BlockIterator *bi = q.top();
    q.pop();
    KV kv = bi->Next();
    if (bi->HasNext())
      q.push(bi);
    if (kv_buffers.size() == 0 || kv.key_.compare(kv_buffers[kv_buffers.size() - 1].key_) > 0) {
      kv_buffers.push_back(kv);
    }
  }
}

void LSMTree::CompactList(size_t level) {
  if (Config::TRACE_LOG)
    std::cout << "CompactList On Level:" << level << std::endl;
  std::vector<Meta> wait_queue_;
  for (size_t i = 0; i < buffer_[level].size(); ++ i)
    wait_queue_.push_back(buffer_[level][i]);
  buffer_[level].clear();
  GetOverlaps(file_[level], wait_queue_);
  std::vector<TableIterator*> tables_;
  for (size_t i = 0; i < wait_queue_.size(); ++ i) {
    std::string filename = GetFilename(wait_queue_[i].sequence_number_);
    tables_.push_back(new TableIterator(filename, filesystem_, filtermanager_, wait_queue_[i], lsmtreeresult_));
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

  std::vector<TableIterator*> tables_;
  std::vector<IndexTableIterator*> index_tables_;
  size_t total_size_ = 0;
  for (size_t i = 0; i < wait_queue_.size(); ++ i) {
    std::string filename = GetFilename(wait_queue_[i].sequence_number_);
    if (algo == std::string("LevelDB-Sep"))
      index_tables_.push_back(new IndexTableIterator(filename, filesystem_, wait_queue_[i], lsmtreeresult_));
    else
      tables_.push_back(new TableIterator(filename, filesystem_, filtermanager_, wait_queue_[i], lsmtreeresult_));
    total_size_ = total_size_ + wait_queue_[i].file_size_;
    filesystem_->Delete(filename);
  }
  lsmtreeresult_->MajorCompaction(total_size_);
  std::vector<Table*> merged_tables;
  if (algo == std::string("LevelDB-Sep"))
    merged_tables = MergeIndexTables(index_tables_);
  else
    merged_tables = MergeTables(tables_);    
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
  std::cout << "Level: " << level << std::endl;
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