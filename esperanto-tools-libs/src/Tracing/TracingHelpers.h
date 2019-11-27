/*------------------------------------------------------------------------------
 * Copyright (C) 2019, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 ------------------------------------------------------------------------------ */

#ifndef ET_RUNTIME_TRACING_HELPERS_H
#define ET_RUNTIME_TRACING_HELPERS_H

#include "esperanto/runtime/CodeManagement/Kernel.h"

#include <iostream>
#include <type_traits>
#include <vector>

/// @brief Helper overload of operator<< that will allow us to print
/// an array of enums
template <class T, typename = std::enable_if_t<std::is_enum_v<T>>>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &vec) {
  for (unsigned int i =  0; i < vec.size(); i++) {
    os << "[" << i << "] = " << static_cast<int64_t>(vec[i]) << ", ";
  }
  return os;
}

std::ostream &operator<<(std::ostream &os,
                         const std::vector<unsigned char> &vec);

/// @brief Helper function that will allow us to convert a vector from one type
/// to another
template <class SrcType, class DstType>
std::vector<DstType> conv_vec(const std::vector<SrcType> &src);

#endif //ET_RUNTIME_TRACING_H
