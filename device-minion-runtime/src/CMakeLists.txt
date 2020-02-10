#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

get_filename_component(SHARED_SRC_DIR "shared/src" ABSOLUTE)
get_filename_component(SHARED_INC_DIR "shared/include" ABSOLUTE)
get_filename_component(SHARED_HAL_DIR "shared/include/etsoc_hal" ABSOLUTE)
set(SHARED_INC_DIRS ${SHARED_INC_DIR} ${SHARED_HAL_DIR})

#FreeRTOS
set(FREERTOS_DIR "shared/FreeRTOS")

#FreeRTOS source
file(GLOB FREERTOS_SRC_FILES "${FREERTOS_DIR}/Source/*.c"
                             "${FREERTOS_DIR}/Source/portable/GCC/RISC-V/*.c"
                             "${FREERTOS_DIR}/Source/portable/GCC/RISC-V/*.S")
                             #"${FREERTOS_DIR}/Source/mem_mang/heap_3.c" not using FreeRTOS heap. Heap is only for libc.

#FreeRTOS includes
get_filename_component(FREERTOS_INC_DIR "${FREERTOS_DIR}/Source/include/" ABSOLUTE)
get_filename_component(FREERTOS_PORT_INC_DIR "${FREERTOS_DIR}/Source/portable/GCC/RISC-V/" ABSOLUTE)
get_filename_component(FREERTOS_CHIP_SPECIFIC_INC_DIR "${FREERTOS_DIR}/Source/portable/GCC/RISC-V/chip_specific_extensions/etsoc_RV64IMF" ABSOLUTE)
set(FREERTOS_INC_DIRS ${FREERTOS_INC_DIR} ${FREERTOS_PORT_INC_DIR} ${FREERTOS_CHIP_SPECIFIC_INC_DIR})

# Build or find the DeviceAPI. This step is moved here, even through the DeviceAPI is consumed only
# by the MM-firmware, because it is also consumed by the shared header and the custom kernels
if("${devicefw_DEVICE_API_PROVIDER}" STREQUAL "module")

  set(device_api_INSTALL ${DEVFW_EXTERNAL_INSTALL_PREFIX})
  execute_process(COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/MasterMinion/device-api)
  execute_process(COMMAND ${CMAKE_COMMAND} -H${CMAKE_CURRENT_SOURCE_DIR}/MasterMinion/device_api
                     "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} -fPIC"
                     -DFLATBUFFERS_BUILD_FLATLIB=ON
                     -DFLATBUFFERS_INSTALL=ON
                     -DCMAKE_INSTALL_PREFIX=${device_api_INSTALL}
                     -DBUILD_DEPENDENCIES=${BUILD_DEPENDENCIES}
                     ${EXTERNAL_PROJECT_CCACHE}
                     ${EXTERNAL_PROJECT_ENABLE_CCACHE}
    RESULT_VARIABLE result
    ERROR_VARIABLE err
    OUTPUT_VARIABLE out
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/MasterMinion/device-api)
  if(result)
    message(FATAL_ERROR "CMake step for device-api failed ${result}: ${err} ${out}")
  endif()
  # Build the device_api target
  execute_process(COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE} --target install
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/MasterMinion/device-api)
  if(result)
    message(FATAL_ERROR "Build step for device-api failed: ${result}")
  endif()
  message(STATUS "Building: device-api...DONE")

  # Force the search location to be the one where we have installed the package
  set(esperanto-device-api_DIR ${device_api_INSTALL}/lib/cmake/esperanto-device-api)
  find_package(esperanto-device-api CONFIG REQUIRED
    PATHS "${device_api_INSTALL}/lib/cmake/esperanto-device-api"
    NO_CMAKE_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_DEFAULT_PATH
    )

else()
  find_package(esperanto-device-api CONFIG REQUIRED)
endif()
message("Esperanto Device API Found: ${esperanto-device-api_FOUND}")

add_subdirectory("shared/")
add_subdirectory("MachineMinion")
add_subdirectory("MasterMinion")
add_subdirectory("WorkerMinion")

add_subdirectory("ServiceProcessorBL1")
add_subdirectory("ServiceProcessorBL2")
add_subdirectory("ServiceProcessor")

if(BUILD_TEST_KERNELS)
  add_subdirectory("kernels")
endif()

install(
  PROGRAMS elftohex.py get_git_hash.py get_git_version.py
  DESTINATION ${BIN_INSTALL_DIR}/esperanto-fw
  COMPONENT tools)
