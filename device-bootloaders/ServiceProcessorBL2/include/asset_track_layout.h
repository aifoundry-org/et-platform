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
#ifndef ASSET_TRACK_LAYOUT_H
#define ASSET_TRACK_LAYOUT_H

#define ASSET_TRACK_PART_NUMBER_OFFSET    16
#define ASSET_TRACK_PART_NUMBER_SIZE      4
#define ASSET_TRACK_SERIAL_NUMBER_OFFSET  (ASSET_TRACK_PART_NUMBER_OFFSET + ASSET_TRACK_PART_NUMBER_SIZE)
#define ASSET_TRACK_SERIAL_NUMBER_SIZE    8
#define ASSET_TRACK_MEMORY_SIZE_OFFSET    (ASSET_TRACK_SERIAL_NUMBER_OFFSET + ASSET_TRACK_SERIAL_NUMBER_SIZE)
#define ASSET_TRACK_MEMORY_SIZE_SIZE      1
#define ASSET_TRACK_MODULE_REV_OFFSET     (ASSET_TRACK_MEMORY_SIZE_OFFSET + ASSET_TRACK_MEMORY_SIZE_SIZE)
#define ASSET_TRACK_MODULE_REV_SIZE       4
#define ASSET_TRACK_FORM_FACTOR_OFFSET    (ASSET_TRACK_MODULE_REV_OFFSET + ASSET_TRACK_MODULE_REV_SIZE)
#define ASSET_TRACK_FORM_FACTOR_SIZE      1

#endif
