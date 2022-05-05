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
find_package(glog REQUIRED)
find_package(Threads REQUIRED)

set(GDB_DIR "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(debug-server
    src/DebugServer.cpp
    src/GdbServer.cpp
    src/MinionDebugInterface.cpp
    src/MiscUtils.cpp
    src/RspConnection.cpp
    src/RspPacket.cpp
    src/MDIEvent.cpp
)

target_compile_features(debug-server PUBLIC cxx_std_17)

target_include_directories(debug-server
    PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tools/debug_server/include>
)

target_link_libraries(debug-server
    PRIVATE
        glog::glog
        ${CMAKE_THREAD_LIBS_INIT}
        ${CMAKE_DL_LIBS}
        deviceManagement::DM
        deviceLayer::deviceLayer
)

install(TARGETS debug-server
    EXPORT deviceManagementTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

#Copy over Debug Context Dump helper infrastructure to install directory
file(COPY debug_context_dump.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY debug_context_dump.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY debug_context_dump_cmd_run.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY debug_data.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY target.xml DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

if (BUILD_TESTS)
   add_subdirectory(tests)
endif(BUILD_TESTS)
