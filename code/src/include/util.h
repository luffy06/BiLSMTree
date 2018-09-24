#include <string>

using namespace std;

class Convertor {
public:
  Convertor();
  ~Convertor();
  
  static string LongToString(long value);

  static long StringToLong(const string &value);
};