

add_library(device_trace INTERFACE)
add_library(deviceFirmware::device_trace ALIAS device_trace)
target_include_directories(device_trace
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        # $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/device-trace>)

install(TARGETS device_trace
    EXPORT EsperantoDeviceMinionRuntimeTargets
    LIBRARY       DESTINATION lib                                       COMPONENT device_trace
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDE_DIR}/esperanto-fw COMPONENT device_trace
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw COMPONENT device_trace
)
