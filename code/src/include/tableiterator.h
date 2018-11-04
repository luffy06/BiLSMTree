namespace bilsmtree {

class TableIterator {
public:
  TableIterator();

  TableIterator(std::string filename);

  ~TableIterator();
  
  bool HasNext();

  KV Next();

  KV Current();

  void SetId(size_t id);

  friend bool operator<(TableIterator* t1, TableIterator* t2) {
    int cp = t1->Current().key_.compare(t2->Current().key_);
    if (cp != 0)
      return (cp > 0 ? true : false)
    else
      return t1->Id() < t2->Id();
  }
private:
  size_t id_;
  std::vector<KV> kvs_;
  size_t iter_;
};
}