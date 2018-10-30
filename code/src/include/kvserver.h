namespace bilsmtree {

class KVServer {
public:
  KVServer();
  
  ~KVServer();

  Slice Get(const Slice& value);

  void MinorCompact(const SkipList* sl);
private:
  LSMTree* lsmtree_;
  LogManager* logmanager_;
};

}