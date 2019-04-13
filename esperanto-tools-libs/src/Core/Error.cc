//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "Core/Error.h"

namespace et_runtime {

const char *Error::errorString(etrtError error) {
  // FIXME SW-254
  // CUDA returns "unrecognized error code" if the error code is not recognized.
  return error == etrtSuccess ? "no error" : "etrt unrecognized error code";
}

} // namespace et_runtime
