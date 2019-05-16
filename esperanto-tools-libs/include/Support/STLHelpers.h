//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_STL_HELPER_H
#define ET_RUNTIME_STL_HELPER_H

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

namespace inner {
template <typename T> void join_helper(std::ostringstream &stream, T &&t) {
  stream << std::forward<T>(t);
}

template <typename T, typename... Ts>
void join_helper(std::ostringstream &stream, T &&t, Ts &&... ts) {
  stream << std::forward<T>(t);
  join_helper(stream, std::forward<Ts>(ts)...);
}
} // namespace inner

// Helper template to avoid lots of nasty string temporary munging.
template <typename... Ts> std::string join(Ts &&... ts) {
  std::ostringstream stream;
  inner::join_helper(stream, std::forward<Ts>(ts)...);
  return stream.str();
}

inline bool is_power_of_2(size_t x) { return x && ((x & (x - 1)) == 0); }

inline size_t align_up(size_t x, size_t alignment) {
  assert(is_power_of_2(alignment));
  return (x + alignment - 1) & ~(alignment - 1);
}

inline bool is_aligned(size_t x, size_t alignment) {
  assert(is_power_of_2(alignment));
  return (x & (alignment - 1)) == 0;
}

template <typename T>
inline std::ptrdiff_t stl_count(const std::vector<T> &v, const T &item) {
  return std::count(v.begin(), v.end(), item);
}

template <typename T> inline void stl_remove(std::vector<T> &v, const T &item) {
  v.erase(std::remove(v.begin(), v.end(), item), v.end());
}

template <typename T>
inline std::ptrdiff_t stl_count(const std::vector<std::unique_ptr<T>> &v,
                                const T *ptr) {
  return std::count_if(
      v.begin(), v.end(),
      [ptr](const std::unique_ptr<T> &item) { return item.get() == ptr; });
}

template <typename T>
inline void stl_remove(std::vector<std::unique_ptr<T>> &v, const T *ptr) {
  v.erase(std::remove_if(v.begin(), v.end(),
                         [ptr](const std::unique_ptr<T> &item) {
                           return item.get() == ptr;
                         }),
          v.end());
}

// FIXME the following  shoud move to helpers only for the grid
inline unsigned defaultGridDim1D(unsigned global_size) {
  return align_up(global_size, 32) / 32;
}
inline unsigned defaultBlockDim1D() { return 32; }

#endif // ET_RUNTIME_STL_HELPERS_H
