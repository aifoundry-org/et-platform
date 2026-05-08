############################
# Erbium peripheral drivers
############################
# Header-only INTERFACE target shipping the L1 driver wrappers
# (ipi/mprot/plic/thread/timer/uart + the Shakti UART IP driver) plus
# the shared mmio helpers. Backend-specific to erbium — addresses /
# bit layouts come from erbium_hal hwinc.
#
# Consumers (erbium-umode kernels, future Zephyr-on-erbium, bring-up
# bootloaders) link et-common-libs::erbium-drivers and get the right
# headers on the include path.

set(ERBIUM_DRIVERS_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/erbium-drivers)

set(ERBIUM_DRIVERS_HDRS
    # Erbium driver headers
    include/erbium/drivers/ipi.h
    include/erbium/drivers/mprot.h
    include/erbium/drivers/plic.h
    include/erbium/drivers/shakti_uart.h
    include/erbium/drivers/thread.h
    include/erbium/drivers/timer.h
    include/erbium/drivers/uart.h
    # Shared helpers
    include/common/mmio.h
)

add_library(erbium-drivers INTERFACE)
add_library(et-common-libs::erbium-drivers ALIAS erbium-drivers)
target_include_directories(erbium-drivers
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${ERBIUM_DRIVERS_INSTALL_PREFIX}/include>
)
target_link_libraries(erbium-drivers INTERFACE erbium_hal::erbium_hal)

# Preserve directory structure on install.
macro(InstallErbiumDriversHdrsWithDirStruct HEADER_LIST)
    foreach(HEADER ${${HEADER_LIST}})
        string(REGEX MATCH "(.*)[/\]" DIR ${HEADER})
        install(FILES ${HEADER} DESTINATION ${ERBIUM_DRIVERS_INSTALL_PREFIX}/${DIR})
    endforeach(HEADER)
endmacro(InstallErbiumDriversHdrsWithDirStruct)

InstallErbiumDriversHdrsWithDirStruct(ERBIUM_DRIVERS_HDRS)

install(
    TARGETS erbium-drivers
    EXPORT erbium-driversTargets
    INCLUDES DESTINATION ${ERBIUM_DRIVERS_INSTALL_PREFIX}/include
)

install(
    EXPORT erbium-driversTargets
    NAMESPACE et-common-libs::
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/et-common-libs/erbium-drivers
    COMPONENT erbium-drivers
)
