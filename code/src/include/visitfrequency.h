namespace bilsmtree {
class VisitFrequencyConfig {
public:
  VisitFrequencyConfig();
  ~VisitFrequencyConfig();

  const static size_t MAXQUEUESIZE = 100000;
  
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

  void Dump(size_t queue_index);

  void Load(size_t queue_index);
};
}