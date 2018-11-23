#include "visitfrequency.h"

namespace bilsmtree {

VisitFrequency::VisitFrequency(size_t max_size, FileSystem* filesystem) {
  filesystem_ = filesystem;
  max_size_ = max_size;
  size_ = 0;
  if (max_size_ > Config::VisitFrequencyConfig::MAXQUEUESIZE * 2) {
    mode_ = 1;
    filesystem_->Create(Config::VisitFrequencyConfig::FREQUENCYPATH);
  }
  else {
    mode_ = 0;
  }
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
      if (visit_[0].size() < Config::VisitFrequencyConfig::MAXQUEUESIZE) {
        visit_[0].push(file_number);
      }
      else if (visit_[1].size() < Config::VisitFrequencyConfig::MAXQUEUESIZE) {
        visit_[1].push(file_number);
      }
      else {
        Dump();
        while (!visit_[1].empty()) visit_[1].pop();
        visit_[1].push(file_number);
      }
    }
    else {
      assert(visit_[0].size() == visit_[1].size());
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
  assert(false);
  size_t file_number_ = filesystem_->Open(Config::VisitFrequencyConfig::FREQUENCYPATH, 
        Config::FileSystemConfig::WRITE_OPTION | Config::FileSystemConfig::APPEND_OPTION);
  while (!visit_[1].empty()) {
    size_t d = visit_[1].front();
    visit_[1].pop();
    std::string data = Util::IntToString(d);
    filesystem_->Write(file_number_, data.data(), data.size());    
  }
  filesystem_->Close(file_number_);
}

void VisitFrequency::Load() {
  assert(false);
  size_t file_number_ = filesystem_->Open(Config::VisitFrequencyConfig::FREQUENCYPATH, 
        Config::FileSystemConfig::READ_OPTION);
  for (size_t i = 0; i < Config::VisitFrequencyConfig::MAXQUEUESIZE; ++ i) {
    std::string data = filesystem_->Read(file_number_, sizeof(size_t));
    visit_[0].push(static_cast<size_t>(Util::StringToInt(data)));
  }
  filesystem_->Close(file_number_);
}

}