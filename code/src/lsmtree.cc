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
        if (deleted != -1) frequency_[deleted] --;
        RollBack(check_files_[j].sequence_number_);
        return value;
      }
    }
  }
  return NULL;
}

void LSMTree::AddTableToL0(const Table* table) {

}

size_t LSMTree::GetSequenceNumber() {
  return total_sequence_number_ ++;
}

std::string LSMTree::GetFilename(size_t sequence_number_) {

}

Slice LSMTree::GetFromFile(const Meta& meta, const Slice& key) {
  std::string filename = GetFilename(meta.sequence_number_);
  int file_number_ = FileSystem.Open(filename, FileSystemConfig::READ_OPTION);
  FileSystem.Seek(file_number_, meta.file_size_ - TableConfig::FOOTERSIZE);
  std::string index_offset_ = FileSystem.Read(file_number_, sizeof(size_t));
  std::string filter_offset_ = FileSystem.Read(file_number_, sizeof(size_t));
  FileSystem.Close(file_number_);
}

void RollBack(size_t sequence_number) {

}

}