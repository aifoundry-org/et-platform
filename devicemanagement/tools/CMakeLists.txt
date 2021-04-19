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

find_package(esperanto-device-api REQUIRED)
find_package(DeviceLayer REQUIRED)
find_package(fmt REQUIRED)
find_package(sw-sysemu REQUIRED)
find_package(Threads REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${ESPERANTO_DEVICE_API_INCLUDE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/../deviceLayer/include)
include_directories(${PROJECT_SOURCE_DIR}/../deviceLayer/src)

link_directories(${RT_EXTERNAL_INSTALL_PREFIX}/lib64
  ${RT_EXTERNAL_INSTALL_PREFIX}/lib
  ${DEPENDENCIES_PATH}/lib64
  ${DEPENDENCIES_PATH}/lib
)

set(COMMON_LIBRARIES
  glog
  pthread
  ${CMAKE_DL_LIBS}
  deviceManagement::deviceManagement
  deviceLayer::deviceLayer
)

add_executable(dm-tester dm_tester.cc)
target_compile_features(dm-tester PUBLIC cxx_std_17)
target_link_libraries(dm-tester ${COMMON_LIBRARIES})

install(TARGETS dm-tester
  EXPORT deviceManagementTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_executable(dm-tester-pcie dm_tester.cc)
target_compile_definitions(dm-tester-pcie PUBLIC TARGET_PCIE)
target_compile_features(dm-tester-pcie PUBLIC cxx_std_17)
set_target_properties(dm-tester-pcie PROPERTIES OUTPUT_NAME "dm-tester-pcie")
target_link_libraries(dm-tester-pcie ${COMMON_LIBRARIES})

install(TARGETS dm-tester-pcie
  EXPORT deviceManagementTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
