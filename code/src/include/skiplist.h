namespace bilsmtree {

class ImmutableMemTableConfig {
public:
  ImmutableMemTableConfig() { }
  ~ImmutableMemTableConfig() { }

  const static int MAXSIZE = 4096;
};

class SkipList {
public:
  SkipList();

  ~SkipList();

  bool IsFull();

  Slice Find(const Slice& key);
  
  void Insert(const KV& kv);

  void Delete(const Slice& key);
private:
  struct ListNode {
    KV kv_;
    int level_;
    ListNode** forward_;
  };

  const double PROB = 0.5;
  const int MAXLEVEL = 4096;
  int data_size_;
  ListNode* head_;

  int GenerateLevel();
};

}