
add_library(etsoc_hal INTERFACE)
add_library(etsoc_hal::etsoc_hal ALIAS etsoc_hal)
target_include_directories(etsoc_hal
    INTERFACE 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/etsoc_hal>
)

install(DIRECTORY etsoc_hal DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw)
install(TARGETS etsoc_hal
    EXPORT EsperantoDeviceMinionRuntimeTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/esperanto-fw/etsoc_hal
    COMPONENT etsoc_hal
)
