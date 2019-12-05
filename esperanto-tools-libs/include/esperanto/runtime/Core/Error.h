//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_ERROR_H
#define ET_RUNTIME_ERROR_H

/// @file

#include "esperanto/runtime/Common/ErrorTypes.h"

namespace et_runtime {

/// @class Error
class Error {
public:
  ///
  /// @brief  Return the description string for an API call return value.
  ///
  /// Take an API return value and return a pointer to the string that describes
  ///  the result of an API call.
  ///
  /// @param[in] error  The value returned by an API call.
  /// @return  A string that describes the result of an API call.
  static const char *errorString(etrtError error);
};

} // namespace et_runtime
#endif // ET_RUNTIME_ERROR_H
