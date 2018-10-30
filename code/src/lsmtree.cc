namespace bilsmtree {

LSMTree::LSMTree() {
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
  }
}

void LSMTree::AddTableToL0(const Table* table) {

}

size_t LSMTree::GetSequenceNumber() {
  return sequence_number_ ++;
}

void RollBack(size_t file_number) {

}

}