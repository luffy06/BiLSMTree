namespace bilsmtree {

class KVServer {
public:
  KVServer();
  
  ~KVServer();

  void Put(const KV& kv);

  Slice Get(const Slice& value);

  void Compact(const SkipList* sl);
private:
  LSMTree* lsmtree_;
  LogManager* logmanager_;
};

}