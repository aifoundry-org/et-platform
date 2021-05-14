add_library(device_serial STATIC 
    src/constant_memory_compare.c
    src/serial.c
    src/sp_otp.c
)
add_library(deviceFirmware::device_serial ALIAS device_serial)
target_include_directories(device_serial
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(device_serial PUBLIC etsoc_hal::etsoc_hal)
