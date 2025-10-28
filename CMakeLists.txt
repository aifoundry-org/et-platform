cmake_minimum_required(VERSION 3.5)
project(etsys)

#
# Allow projects to find cmake directory in source and installed paths
#
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake-modules)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_INSTALL_PREFIX}/lib/cmake)

configure_file(cmake/riscv64-ec-toolchain.cmake.in ${CMAKE_INSTALL_PREFIX}/lib/cmake/riscv64-ec-toolchain.cmake @ONLY)

set(ET_PLATFORM_PATH ${CMAKE_INSTALL_DIR})
include(ProjectFunctions)

#
# Install CMake files useful for compiling subprojects.
#
file(COPY cmake/ProjectFunctions.cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/aifoundry-utils)
file(COPY cmake/Findlibcap.cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake)
file(COPY cmake/Findgflags.cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake)
file(COPY cmake-modules DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake)

# External dependencies
ThirdParty(g3log https://github.com/KjellKod/g3log v1.1-408-g38dbddc "-DUSE_DYNAMIC_LOGGING_LEVELS=ON -DADD_G3LOG_UNIT_TEST=OFF -DINSTALL_G3LOG=ON")
ThirdParty(easy_profiler https://github.com/yse/easy_profiler.git v2.1.0-66-gcc0e154 "")
ThirdParty(cereal https://github.com/USCiLab/cereal.git v1.3.2 "-DJUST_INSTALL_CEREAL=ON")

#
# Install the public headers of the linux driver.
#
HostProject(et-driver "")

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
HostProject(sw-sysemu "" g3log common-sw et-driver)
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
