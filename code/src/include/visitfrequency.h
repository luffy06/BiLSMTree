#ifndef BILSMTREE_VISITFREQUENCY_H
#define BILSMTREE_VISITFREQUENCY_H

#include <queue>

namespace bilsmtree {

class FileSystem;

class VisitFrequencyConfig {
public:
  VisitFrequencyConfig();
  ~VisitFrequencyConfig();

  const static size_t MAXQUEUESIZE = 100000;
  const static std::string FREQUENCYPATH = "../logs/frequency.log"
};

class VisitFrequency {
public:
  VisitFrequency() { }

  VisitFrequency(size_t max_size);

  ~VisitFrequency();

  int Append(size_t file_number);
private:
  std::queue<size_t> visit_[2];

  size_t max_size_;
  size_t size_;

  size_t mode_; // 0 for small queue 1 for big queue 

  void Dump();

  void Load();
};
}

#endif