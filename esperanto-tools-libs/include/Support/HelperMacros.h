//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_HELPER_MACROS_H
#define ET_RUNTIME_HELPER_MACROS_H

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include <cstring>
#include <string>

#define STRINGIFY(s) STRINGIFY_INTERNAL(s)
#define STRINGIFY_INTERNAL(s) #s

#define PERROR_IF(fail_cond)                                                   \
  do {                                                                         \
    if (fail_cond) {                                                           \
      fprintf(stderr, __FILE__ ":" STRINGIFY(__LINE__) ": %s: PERROR: %s\n",   \
              __PRETTY_FUNCTION__, strerror(errno));                           \
      abort();                                                                 \
    }                                                                          \
  } while (0)

[[noreturn]] inline void THROW(std::string msg) {
  fprintf(stderr, "There was an error: %s\n", msg.c_str());
  abort();
}

inline void THROW_IF(bool fail_cond, std::string msg) {
  if (fail_cond) {
    THROW(msg);
  }
}

#endif // ET_RUNTIME_HELPER_MACROS_H
