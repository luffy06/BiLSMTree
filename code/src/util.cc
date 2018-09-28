#include "util.h"

Convertor::Convertor() {

}

Convertor::~Convertor() {

}

std::string Convertor::LongToString(long value) {
  char buf[100];
  sprintf(buf, "%ld", value);
  return std::string(buf);
}

long Convertor::StringToLong(const std::string &value) {
  long buf;
  sscanf(value.c_str(), "%ld", &buf);
  return buf;
}