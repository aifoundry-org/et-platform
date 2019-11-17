#ifndef ETTEE_DEMANGLE_H
#define ETTEE_DEMANGLE_H

#include <cxxabi.h>
#include <string>

inline std::string demangle(const char *mangled_name) {
  int status;
  char *realname;
  std::string res;

  realname = abi::__cxa_demangle(mangled_name, 0, 0, &status);
  if (realname) {
    res.assign(realname);
  }
  free(realname);

  return res;
}

inline std::string demangle(const std::string &s) {
  return demangle(s.c_str());
};
  // https://gcc.gnu.org/onlinedocs/libstdc++/manual/ext_demangling.html

#endif // ETTEE_DEMANGLE_H
