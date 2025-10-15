# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(include)

add_library(minion_rt INTERFACE)
add_library(deviceFirmware::minion_rt_helpers_interface ALIAS minion_rt)
target_include_directories(minion_rt
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw>
)

target_link_libraries(minion_rt INTERFACE
    signedImageFormat::signedImageFormat
)
