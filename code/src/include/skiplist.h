namespace bilsmtree {

class SkipList {
public:
  SkipList();
  ~SkipList();
  

  void Insert(const Slice& key, const Slice& value);

  Slice Find(const Slice& key);
};

}