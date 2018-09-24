#include "util.h"

Convertor::Convertor() {

}

Convertor::~Convertor() {

}

string Convertor::LongToString(long value) {
  char buf[100];
  sprintf(buf, "%ld", value);
  return string(buf);
}

long Convertor::StringToLong(const string &value) {
  long buf;
  sscanf(value.c_str(), "%ld", &buf);
  return buf;
}