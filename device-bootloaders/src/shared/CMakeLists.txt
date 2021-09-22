add_library(device_common STATIC 
    src/constant_memory_compare.c
    src/sp_otp.c
)
add_library(deviceFirmware::device_common ALIAS device_common)
target_include_directories(device_common
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(device_common 
    PUBLIC 
        etsoc_hal::etsoc_hal
        et-common-libs::sp-bl2
)
