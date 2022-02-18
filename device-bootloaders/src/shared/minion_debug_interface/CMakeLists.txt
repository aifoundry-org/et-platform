# nobody is using minion_debug_interface target ... 

add_library(minion_debug_interface
    src/debug_accessor.c
    src/minion_run_control.c
    src/minion_state_inspection.c
)
add_library(esperantoBootLoader::minion_debug_interface ALIAS minion_debug_interface)
target_include_directories(minion_debug_interface
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(minion_debug_interface 
    PUBLIC
        deviceApi::deviceApi
        etsoc_hal::etsoc_hal
        et-common-libs::sp-bl2
        
        esperantoBootLoader::common
)
