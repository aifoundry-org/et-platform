############################
# ETSoC1 peripheral drivers
############################
# Header-only INTERFACE target shipping the L1 driver wrappers
# (ipi/mprot/plic/thread/timer/uart) ported from the erbium side.
# Backend-specific to ETSoC1 — addresses come from etsoc_hal hwinc
# (etsoc_neigh_esr.h, etsoc_shire_other_esr.h, hal_device.h, ...).
#
# The legacy etsoc/drivers/{pcie,pmu,serial} headers continue to
# ship via the existing cm_rt_svcs / mm_rt_svcs / minion_bl /
# sp_bl* targets and are not pulled into this package.
#
# Consumers (firmware, future Zephyr-on-etsoc) link
# et-common-libs::etsoc-drivers and get the right headers on the
# include path.

set(ETSOC_DRIVERS_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/etsoc-drivers)

set(ETSOC_DRIVERS_HDRS
    # ETSoC1 driver headers
    include/etsoc/drivers/ipi.h
    include/etsoc/drivers/mprot.h
    include/etsoc/drivers/plic.h
    include/etsoc/drivers/thread.h
    include/etsoc/drivers/timer.h
    include/etsoc/drivers/uart.h
    # Shared helpers
    include/common/mmio.h
)

add_library(etsoc-drivers INTERFACE)
add_library(et-common-libs::etsoc-drivers ALIAS etsoc-drivers)
target_include_directories(etsoc-drivers
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${ETSOC_DRIVERS_INSTALL_PREFIX}/include>
)
target_link_libraries(etsoc-drivers INTERFACE etsoc_hal::etsoc_hal)

# Preserve directory structure on install.
macro(InstallEtsocDriversHdrsWithDirStruct HEADER_LIST)
    foreach(HEADER ${${HEADER_LIST}})
        string(REGEX MATCH "(.*)[/\]" DIR ${HEADER})
        install(FILES ${HEADER} DESTINATION ${ETSOC_DRIVERS_INSTALL_PREFIX}/${DIR})
    endforeach(HEADER)
endmacro(InstallEtsocDriversHdrsWithDirStruct)

InstallEtsocDriversHdrsWithDirStruct(ETSOC_DRIVERS_HDRS)

install(
    TARGETS etsoc-drivers
    EXPORT etsoc-driversTargets
    INCLUDES DESTINATION ${ETSOC_DRIVERS_INSTALL_PREFIX}/include
)

install(
    EXPORT etsoc-driversTargets
    NAMESPACE et-common-libs::
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/et-common-libs/etsoc-drivers
    COMPONENT etsoc-drivers
)
