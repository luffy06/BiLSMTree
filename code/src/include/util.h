#include <string>
#include <fstream>

namespace bilsmtree {

class GlobalConfig {
public:
  GlobalConfig();

  ~GlobalConfig();
  
  enum Algorithms {
    LevelDB,
    BiLSMTree
  };

  Algorithms algorithm;
};

class Util {
public:
  Util();
  ~Util();
  
  static std::string LongToString(long value);

  static long StringToLong(const std::string &value);

  static bool ExistFile(const std::string &filename);

  static void Assert(const char* message, bool condition);
};

class Config {
public:
  Config();
  ~Config();
  
  const static bool PERSISTENCE = false;
  const static int READ_OPTION = 1;
  const static int WRITE_OPTION = 1 << 1;
  const static int APPEND_OPTION = 1 << 2;
};

}