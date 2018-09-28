#include <string>

class Convertor {
public:
  Convertor();
  ~Convertor();
  
  static std::string LongToString(long value);

  static long StringToLong(const std::string &value);
};