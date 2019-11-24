/**
 * Copyright (C) 2018, Esperanto Technologies Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file etrt-bin.h
 * @brief ET Runtime Library Includes
 * @version 0.1.0
 *
 * @ingroup ETCRT_API
 *
 * ????
 **/

#ifndef ETRT_BIN_H
#define ETRT_BIN_H

#include "esperanto/runtime/Common/ErrorTypes.h"
#include "esperanto/runtime/Common/ProjectAutogen.h"
#include "esperanto/runtime/Core/Stream.h"

#include <stddef.h>


/**
 * @brief ET Runtime memory types
 */
enum etrtMemoryType {
  etrtMemoryTypeHost = 1,  /**< Host memory */
  etrtMemoryTypeDevice = 2 /**< Device memory */
};

/**
 * @brief ET Runtime memory copy types
 */
enum etrtMemcpyKind {
  etrtMemcpyHostToHost = 0,     /**< Host   -> Host */
  etrtMemcpyHostToDevice = 1,   /**< Host   -> Device */
  etrtMemcpyDeviceToHost = 2,   /**< Device -> Host */
  etrtMemcpyDeviceToDevice = 3, /**< Device -> Device */
  etrtMemcpyDefault =
      4 /**< Direction of the transfer is inferred from the pointer values.
           Requires unified virtual addressing */
};

/**
 * @brief ET Runtime pointer attributes
 */
struct etrtPointerAttributes {
  /**
   * The physical location of the memory, ::etrtMemoryTypeHost or
   * ::etrtMemoryTypeDevice.
   */
  enum etrtMemoryType memoryType;

  /**
   * The device against which the memory was allocated or registered.
   * If the memory type is ::etrtMemoryTypeDevice then this identifies
   * the device on which the memory referred physically resides.  If
   * the memory type is ::etrtMemoryTypeHost then this identifies the
   * device which was current when the memory was allocated or registered
   * (and if that device is deinitialized then this allocation will vanish
   * with that device's state).
   */
  int device;

  /**
   * The address which may be dereferenced on the current device to access
   * the memory or NULL if no such address exists.
   */
  void *devicePointer;

  /**
   * The address which may be dereferenced on the host to access the
   * memory or NULL if no such address exists.
   */
  void *hostPointer;

  /**
   * Indicates if this pointer points to managed memory
   */
  int isManaged;
};

#endif // ETRT_BIN_H
