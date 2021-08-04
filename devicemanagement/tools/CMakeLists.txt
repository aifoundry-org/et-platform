#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

include(ExternalProject)

find_package(deviceLayer REQUIRED)
find_package(gflags REQUIRED)
find_package(glog REQUIRED)
find_package(Threads REQUIRED)

set(COMMON_LIBRARIES
    glog::glog
    gflags::gflags
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
    deviceManagement::DM
    deviceLayer::deviceLayer
)

add_executable(dm-tester dm_tester.cc)
target_compile_features(dm-tester PUBLIC cxx_std_17)
target_include_directories(dm-tester
    PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
)
target_link_libraries(dm-tester ${COMMON_LIBRARIES})

install(TARGETS dm-tester
    EXPORT deviceManagementTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
