#include "Location.h"


void Location::setType(KIND t) {
  type = t;
}

void Location::setValue(VALUE v) {
  value = v;
}

void Location::setLocal(bool l) {
  local = l;
}

KIND Location::getType() {
  return type;
}

VALUE Location::getValue() {
  return value;
}

bool Location::getLocal() {
  return local;
}

string Location::toString() {
  std::stringstream s;

  if (local) {
    s << "LOCAL\t";
  }
  else {
    s << "REGISTER\t";
  }

  switch(type) {
  case PTR_KIND:
    s << "[PTR: " << value.as_ptr << "]";
    break;
  case INT1_KIND:
    s << "[INT1: " << value.as_int << "]";
    break;
  case INT8_KIND:
    s << "[INT8: " << value.as_int << "]";
    break;
  case INT16_KIND:
    s << "[INT16: " << value.as_int << "]";
    break;
  case INT32_KIND:
    s << "[INT32: " << value.as_int << "]";
    break;
  case INT64_KIND:
    s << "[INT64: " << value.as_int << "]";
    break;
  case FLP32_KIND:
    s << "[FLP32: " << value.as_flp << "]";
    break;
  case FLP64_KIND:
    s << "[FLP64: " << value.as_flp << "]";
    break;
  default: //safe_assert(false);
    break;
  }

  return s.str();
}
