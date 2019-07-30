/**
 * Copyright (c) 2018-present, Esperanto Technologies Inc.
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
 */

#ifndef ET_RUNTIME_SUPPORT_TIMEHELPERS_H
#define ET_RUNTIME_SUPPORT_TIMEHELPERS_H

#include <chrono>

namespace et_runtime {

// Top-level aliases of time related types to be used
// in our code-base

using Clock = std::chrono::system_clock;
using Timepoint = Clock::time_point;
using TimeDuration = Clock::duration;

} // namespace et_runtime
#endif // ET_RUNTIME_SUPPORT_TIMEHELPERS_H
