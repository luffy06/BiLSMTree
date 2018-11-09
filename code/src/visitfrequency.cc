#include "visitfrequency.h"

namespace bilsmtree {

VisitFrequency::VisitFrequency(uint max_size) {
  max_size_ = max_size;
  size_ = 0;
  if (max_size_ > Config::VisitFrequencyConfig::MAXQUEUESIZE * 2)
    mode_ = 1;
  else
    mode_ = 0;
}

VisitFrequency::~VisitFrequency() {
  max_size_ = size_ = 0;
}

int VisitFrequency::Append(uint file_number) {
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
  uint file_number_ = FileSystem::Open(Config::VisitFrequencyConfig::FREQUENCYPATH, 
        Config::FileSystemConfig::WRITE_OPTION | Config::FileSystemConfig::APPEND_OPTION);
  while (!visit_[1].empty()) {
    uint d = visit_[1].front();
    visit_[1].pop();
    std::string data = Util::IntToString(d);
    FileSystem::Write(file_number_, data.data(), data.size());    
  }
  FileSystem::Close(file_number_);
}

void VisitFrequency::Load() {
  uint file_number_ = FileSystem::Open(Config::VisitFrequencyConfig::FREQUENCYPATH, 
        Config::FileSystemConfig::READ_OPTION);
  for (uint i = 0; i < Config::VisitFrequencyConfig::MAXQUEUESIZE; ++ i) {
    std::string data = FileSystem::Read(file_number_, sizeof(uint));
    visit_[0].push(static_cast<uint>(Util::StringToInt(data)));
  }
  FileSystem::Close(file_number_);
}

}