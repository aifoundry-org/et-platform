find_package(et-common-libs REQUIRED)

add_library(minion_debug_interface
    src/debug_accessor.c
    src/minion_run_control.c
    src/minion_state_inspection.c
)
add_library(deviceFirmware::minion_debug_interface ALIAS minion_debug_interface)
target_include_directories(minion_debug_interface
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(minion_debug_interface 
    PUBLIC 
        etsoc_hal::etsoc_hal
        esperantoTrace::et_trace
)
