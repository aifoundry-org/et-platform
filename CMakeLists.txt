cmake_minimum_required(VERSION 3.5)
project(etsys)

# Configure device toolchain.
if (NOT DEFINED TOOLCHAIN_DIR)
   set(TOOLCHAIN_DIR "/opt/nekko")
endif()

set(STAGING_DIR ${CMAKE_INSTALL_PREFIX})

configure_file(cmake/riscv64-ec-toolchain.cmake.in ${STAGING_DIR}/lib/cmake/riscv64-ec-toolchain.cmake @ONLY)

#
# Allow projects to find cmake directory in source and installed paths
#
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake-modules)
list(APPEND CMAKE_MODULE_PATH ${STAGING_DIR}/lib/cmake)
set(EXTERNAL_CACHE_FILE "${CMAKE_BINARY_DIR}/external_cache.cmake")
file(WRITE "${EXTERNAL_CACHE_FILE}"
    "set(CMAKE_MODULE_PATH \"${CMAKE_MODULE_PATH}\" CACHE PATH \"Module paths for external project\" FORCE)\n"
)

include(ProjectFunctions)


#
# Manually install linux kernel driver ioctl.h
#
file(COPY et-driver/et_ioctl.h DESTINATION ${STAGING_DIR}/include)


# External dependencies
ThirdParty(g3log https://github.com/KjellKod/g3log v1.1-408-g38dbddc "-DUSE_DYNAMIC_LOGGING_LEVELS=ON -DADD_G3LOG_UNIT_TEST=OFF -DINSTALL_G3LOG=ON")
ThirdParty(easy_profiler https://github.com/yse/easy_profiler.git v2.1.0-66-gcc0e154 "")
ThirdParty(cereal https://github.com/USCiLab/cereal.git v1.3.2 "-DJUST_INSTALL_CEREAL=ON")

#
# Common Software
#
HostProject(common-sw "" g3log)
HostProject(device-api "" common-sw)
DeviceProject(etsoc-hal "")
DeviceProject(et-trace "")
DeviceProject(et-common-libs "-DENABLE_WARNINGS_AS_ERRORS=OFF" et-trace etsoc-hal)

#
# Firmware
#
HostProject(tf-protocol "")
HostProject(firmware-flash-tool "")
DeviceProject(signedImageFormat "")

DeviceProject(device-bootloaders
	"-DBUILD_DOC=OFF -DENABLE_WARNINGS_AS_ERRORS=OFF"
	signedImageFormat device-api firmware-flash-tool
	tf-protocol etsoc-hal et-trace et-common-libs
)

DeviceProject(device-minion-runtime
	"-DENABLE_CMD_EXECUTION_TRACE=OFF"
	device-api signedImageFormat et-trace
	etsoc-hal et-common-libs
	et-common-libs
)

#
# Test kernels
#
DeviceProject(test-compute-kernels ""
	et-common-libs et-trace
)

#
# Runtime
#
HostProject(sw-sysemu "" g3log common-sw)
HostProject(devicelayer  "" common-sw sw-sysemu)
HostProject(esperanto-tools-libs ""
            devicelayer device-api g3log cereal easy_profiler
	    device-bootloaders device-minion-runtime
)

#
# Device Management and Control
#
HostProject(devicemanagement "" device-api common-sw devicelayer)
HostProject(device-management-application "" common-sw g3log devicelayer devicemanagement et-trace)
