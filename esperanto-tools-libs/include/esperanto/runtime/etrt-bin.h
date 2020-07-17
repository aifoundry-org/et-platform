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

#endif // ETRT_BIN_H
