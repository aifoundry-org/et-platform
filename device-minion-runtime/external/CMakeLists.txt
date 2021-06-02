
add_library(etsoc_hal INTERFACE)
add_library(etsoc_hal::etsoc_hal ALIAS etsoc_hal)
target_include_directories(etsoc_hal
    INTERFACE 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/minion/etsoc_hal>
)

# This include directory should eventuall go away when using the single source of etsoc_hal
#   ${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/minion
install(DIRECTORY etsoc_hal DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw/minion)
install(TARGETS etsoc_hal
    EXPORT EsperantoDeviceMinionRuntimeTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/esperanto-fw/etsoc_hal
    COMPONENT etsoc_hal
)
