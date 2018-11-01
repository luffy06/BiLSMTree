namespace bilsmtree {

LSMTree::LSMTree() {
  total_sequence_number_ = 0;
  recent_files_ = new VisitFrequency();
}

LSMTree::~LSMTree() {
  delete recent_files_;
}

Slice LSMTree::Get(const Slice& key) {
  for (size_t i = 0; i < LSMTreeConfig::LEVEL; ++ i) {
    std::vector<Meta> check_files_;
    if (i == 0) {
      for (size_t j = 0; j < file_[i].size(); ++ j) {
        if (key.compare(file_[i][j].smallest_) >= 0 && key.compare(file_[i][j].largest_) <= 0)
          check_files_.push_back(file_[i][j]);
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
        check_files_.push_back(file_[i][r]);
    }
    for (size_t j = 0; j < check_files_.size(); ++ j) {
      Slice value = GetFromFile(check_files_[j], key);
      if (value != NULL) {
        int deleted = recent_files_->Append(check_files_[j].sequence_number_);
        frequency_[check_files_[j].sequence_number_] ++;
        if (deleted != -1) frequency_[deleted.sequence_number_] --;
        // TODO: RUN BY ALPHA
        RollBack(i, check_files_[j]);
        return value;
      }
    }
  }
  return NULL;
}

void LSMTree::AddTableToL0(const Table* table) {
  size_t sequence_number_ = GetSequenceNumber();
  std::string filename = GetFilename(sequence_number_);
  table -> DumpToFile(filename);
  Meta meta = table -> GetMeta();
  meta.sequence_number_ = sequence_number_;
  file_[0].insert(file_[0].begin(), meta);
}

size_t LSMTree::GetSequenceNumber() {
  return total_sequence_number_ ++;
}

std::string LSMTree::GetFilename(size_t sequence_number_) {
  char filename[30];
  sprintf(filename, "%s/%s_%d.bdb", TableConfig::TABLEPATH,data(), TableConfig::TABLENAME.data(), sequence_number_);
  return std::string(filename);
}

Slice LSMTree::GetFromFile(const Meta& meta, const Slice& key) {
  std::string filename = GetFilename(meta.sequence_number_);
  int file_number_ = FileSystem::Open(filename, FileSystem::onfig::READ_OPTION);
  FileSystem::Seek(file_number_, meta.file_size_ - TableConfig::FOOTERSIZE);
  size_t index_offset_ = static_cast<size_t>(Util::StringToLong(FileSystem::Read(file_number_, sizeof(size_t))));
  size_t filter_offset_ = static_cast<size_t>(Util::StringToLong(FileSystem::Read(file_number_, sizeof(size_t))));
  // READ FILTER
  FileSystem::Seek(file_number_, filter_offset_);
  std::string filter_data_ = FileSystem::Read(file_number_, TableConfig::FILTERSIZE);
  Filter* filter;
  if (GlobalConfig::algorithm == GlobalConfig::LevelDB)
    filter = new BloomFilter(filter_data_);
  else if (GlobalConfig::algorithm == GlobalConfig::BiLSMTree)
    filter = new CuckooFilter(filter_data_);
  else
    Util::Assert("Algorithm Error", false);
  if (!filter->KeyMatch(key)) {
    FileSystem::Close(file_number_);
    return NULL;
  }
  // READ INDEX BLOCK
  FileSystem::Seek(file_number_, index_offset_);
  std::string index_data_ = FileSystem::Read(file_number_, TableConfig::BLOCKSIZE);
  std::istringstream is;
  is.open(index_data_);
  std::string key_;
  size_t offset_ = -1;
  char temp[1000];
  while (!is.eof()) {
    is.read(temp, sizeof(size_t));
    size_t key_size_ = Util::StringToLong(std::string(temp));
    is.read(temp, key_size_);
    key_ = std::string(temp);
    is.read(temp, sizeof(size_t));
    offset_ = Util::StringToLong(std::string(temp));
    if (key.compare(key_) <= 0)
      break;
  }
  if (offset_ == -1) {
    FileSystem::Close(file_number_);
    return NULL;
  }
  // READ DATA BLOCK
  FileSystem::Seek(file_number_, offset_);
  std::string data_ = FileSystem::Read(file_number_, TableConfig::BLOCKSIZE);
  FileSystem::Close(file_number_);
  is.open(data_);
  std::string value_;
  while (!is.eof()) {
    is.read(temp, sizeof(size_t));
    size_t key_size_ = Util::StringToLong(std::string(temp));
    is.read(temp, key_size_);
    key_ = std::string(temp);
    is.read(temp, sizeof(size_t));
    size_t value_size_ = Util::StringToLong(std::string(temp));
    is.read(temp, value_size_);
    value_ = std::string(temp);
    if (key.compare(key_) == 0)
      return value_;
  }
  return NULL;
}

size_t GetTargetLevel(const size_t now_level, const Meta& meta) {
  size_t overlaps[7];
  overlaps[now_level] = 0;
  for (int i = now_level - 1; i >= 0; -- i) {
    overlaps[i] = overlaps[i  + 1];
    for (size_t j = 0; j < file_[i].size(); ++ j) {
      if (meta.largest_.compare(file_[i][j].smallest_) >= 0 || meta.smallest_.compare(file_[i][j].largest_) <= 0)
        overlaps[i] = overlaps[i] + 1;
    }
  }
  size_t target_level = now_level;
  double diff = 0;
  for (int i = now_level - 1; i > 0; -- i) {
    double diff_t = double diff = frequency_[meta.sequence_number_] * 3 * (now_level - i) - FlashConfig::READ_WRITE_RATE - overlaps[i];
    if (diff_t > diff) {
      target_level = i;
      diff = diff_t;
    }
  }
  return target_level;
}

void RollBack(const size_t now_level, const Meta& meta) {
  if (now_level == 0)
    return ;
  size_t to_level = GetTargetLevel(now_level, meta);
  if (to_level == now_level)
    return ;
  std::string filename = GetFilename(meta.sequence_number_);
  int file_number_ = FileSystem::Open(filename, FileSystem::onfig::READ_OPTION);
  FileSystem::Seek(file_number_, meta.file_size_ - TableConfig::FOOTERSIZE);
  size_t index_offset_ = static_cast<size_t>(Util::StringToLong(FileSystem::Read(file_number_, sizeof(size_t))));
  size_t filter_offset_ = static_cast<size_t>(Util::StringToLong(FileSystem::Read(file_number_, sizeof(size_t))));
  // MEGER FILTER
  FileSystem::Seek(file_number_, filter_offset_);
  std::string filter_data_ = FileSystem::Read(file_number_, TableConfig::FILTERSIZE);
  Filter* filter;
  if (GlobalConfig::algorithm == GlobalConfig::BiLSMTree)
    filter = new CuckooFilter(filter_data_);
  else
    Util::Assert("Algorithm Error", false);
  for (int i = now_level - 1; i >= to_level; -- i) {
    size_t l = 0;
    int r = file_[i].size() - 1;
    while (l < file_[i].size() && meta.smallest_.compare(file[i][l].largest_) > 0) ++ l;
    while (r >= 0 && meta.largest_.compare(file[i][l].smallest_) < 0) -- l;
    if (l > r) 
      continue;
    for (size_t j = l; j <= r; ++ j) {
      Meta to_meta = file_[i][j];
      std::string to_filename = GetFilename(to_meta.sequence_number_);
      int to_file_number_ = FileSystem::Open(to_filename, FileSystem::onfig::READ_OPTION);
      FileSystem::Seek(to_file_number_, to_meta.file_size_ - TableConfig::FOOTERSIZE);
      size_t to_index_offset_ = static_cast<size_t>(Util::StringToLong(FileSystem::Read(to_file_number_, sizeof(size_t))));
      size_t to_filter_offset_ = static_cast<size_t>(Util::StringToLong(FileSystem::Read(to_file_number_, sizeof(size_t))));
      FileSystem::Seek(to_file_number_, to_filter_offset_);
      std::string to_filter_data_ = FileSystem::Read(to_file_number_, TableConfig::FILTERSIZE);
      Filter* to_filter = new CuckooFilter(to_filter_data_);
      filter->Diff(to_filter);
      FileSystem::Close(to_file_number_);
    }
  }
  FileSystem::Seek(file_number_, filter_offset_);
  filter_data_ = filter->ToString();
  FileSystem::Write(file_number_, filter_data_.data(), filter_data_.size());
  FileSystem::Close(file_number_);
}

}