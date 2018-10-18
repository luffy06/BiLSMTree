namespace bilsmtree {


class BiList {
public:
  BiList(uint size);
  
  ~BiList();
  
  void Set(int pos, KV kv);

  void MoveToHead(int pos);

  KV Get(int pos);

  KV Delete(int pos);

  // insert after tail
  void Append(KV kv);

  // insert before head
  KV Insert(KV kv);

  int Head() { return data[head_].next_; }

  int Tail() { return tail_; }

private:
  struct ListNode {
    KV kv_;
    int next_, prev_;

    Node() { }

    Node(Slice& key, Slice& value) : kv_(key, value) { }
  };

  int head_;        // refer to the head of list which doesn't store data
  int tail_;        // refer to last data's position
  int max_size_;
  int data_size_;
  queue<int> free_;
  ListNode* data_;
};

}