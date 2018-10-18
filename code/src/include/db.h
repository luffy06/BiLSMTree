namespace bilsmtree {

class DB {
public:
  DB();
  ~DB();
private:
  CacherServer* cacherserver;
  KVServer* kvserver;
};

}