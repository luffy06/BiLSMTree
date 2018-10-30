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
        Dump();
        visit_[1].clear();
        visit_[1].push(file_number);
      }
    }
    else {
      Util::Assert("Head size is not equal Tail", (visit_[0].size() == visit_[1].size()));
      if (visit_[0].empty()) {
        Dump();
        Load();
      }
      res = visit_[0].front();
      visit_[0].pop();
      visit_[1].push(file_number);
    }
  }
  size_ ++;
  return res;
}

void VisitFrequency::Dump() {
  int file_number = FileSystem.Open(FREQUENCYPATH, FileSystemConfig::WRITE_OPTION | FileSystemConfig::APPEND_OPTION);
  while (!visit_[1].empty()) {
    size_t d = visit_[1].front();
    visit_[1].pop();
    std::string data = Util::LongToString(d);
    FileSystem.Write(file_number, data.data(), data.size());    
  }
  FileSystem.Close(file_number);
}

void VisitFrequency::Load() {
  int file_number = FileSystem.Open(FREQUENCYPATH, FileSystemConfig::READ_OPTION);
  std::string data;
  for (size_t i = 0; i < VisitFrequencyConfig::MAXQUEUESIZE; ++ i) {
    FileSystem.Read(file_number, data, sizeof(size_t));
    visit_[0].push(static_cast<size_t>(Util::StringToLong(data)));
  }
  FileSystem.Close(file_number);
}

}