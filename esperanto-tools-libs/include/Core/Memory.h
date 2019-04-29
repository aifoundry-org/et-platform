//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_MEMORY_H
#define ET_RUNTIME_MEMORY_H

#include <memory>

namespace et_runtime {

class PointerAttributes;

class AbstractMemoryPtr {
};

/// Class holding a pointer to Host allocated memory.
/// The memory has been allocated through the device and it is
/// pineed on the cost.
class HostMemoryPtr final : public AbstractMemoryPtr {

public:
  PointerAttributes getAttributes();
};

class DeviceMemoryPtr final : public AbstractMemoryPtr {
public:
  PointerAttributes getAttributes();
};

}; // namespace et_runtime

#endif // ET_RUNTIME_MEMORY_H
