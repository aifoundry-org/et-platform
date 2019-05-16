//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_CODE_MODULE_H
#define ET_RUNTIME_CODE_MODULE_H

#include <unordered_map>

namespace et_runtime {

/// @brief Dynamically loaded module descriptor.
class Module {
  // FIXME provide a proper interface to the module members
public:
  std::unordered_map<std::string, size_t>
      kernel_offset; ///< kernel name -> kernel entry point offset
  std::unordered_map<std::string, size_t>
      raw_kernel_offset; ///< raw-kernel name -> kernel entry point offset
};

} // namespace et_runtime

#endif // ET_RUNTIME_CODE_MODULE_H
