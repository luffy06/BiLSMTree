#include "util.h"

namespace bilsmtree {

Util::Util() {

}

Util::~Util() {

}

std::string Util::LongToString(long value) {
  char buf[100];
  sprintf(buf, "%ld", value);
  return std::string(buf);
}

long Util::StringToLong(const std::string &value) {
  long buf;
  sscanf(value.c_str(), "%ld", &buf);
  return buf;
}

bool Util::ExistFile(const std::string &filename) {
  std::fstream f(filename, std::ios::in);
  bool res = f.is_open();
  f.close();
  return res;
}

void Uitl::Assert(const char* message, bool condition) {
  if (!condition)
    printf("%s\n", message);
  assert(condition);
}

}