#------------------------------------------------------------------------------
# Copyright (C) 2021, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

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
