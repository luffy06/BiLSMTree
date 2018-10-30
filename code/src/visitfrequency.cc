namespace bilsmtree {

VisitFrequency::VisitFrequency(size_t max_size) {
  max_size_ = max_size;
  size_ = 0;
  if (max_size_ > VisitFrequencyConfig::MAXQUEUESIZE * 2)
    mode_ = 1;
  else
    mode_ = 0;
}

VisitFrequency::~VisitFrequency() {
  max_size_ = size_ = 0;
}

int VisitFrequency::Append(size_t file_number) {
  int res = -1;
  if (mode_ == 0) {
    if (size_ < max_size_) {
      visit_[0].push(file_number);
    }
    else {
      visit_[0].push(file_number);
      res = visit_[0].front();
      visit_[0].pop();
    }
  }
  else {
    if (size_ < max_size_) {
      if (visit_[0].size() < VisitFrequencyConfig::MAXQUEUESIZE) {
        visit_[0].push(file_number);
      }
      else if (visit_[1].size() < VisitFrequencyConfig::MAXQUEUESIZE) {
        visit_[1].push(file_number);
      }
      else {
        Dump(1);
        visit_[1].clear();
        visit_[1].push(file_number);
      }
    }
    else {
      Util::Assert("Head size is not equal Tail", (visit_[0].size() == visit_[1].size()));
      if (visit_[0].empty()) {
        Dump(1);
        Load(0);
      }
      res = visit_[0].front();
      visit_[0].pop();
      visit_[1].push(file_number);
    }
  }
  size_ ++;
  return res;
}

}