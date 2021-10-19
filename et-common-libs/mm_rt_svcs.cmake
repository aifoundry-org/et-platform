##########################
# Minion Run Time Services
##########################

#Minion Boot Loader requires etsoc_hal library
find_package(etsoc_hal REQUIRED)

#Install prefix for Minion Bootloader library
set(MM_RT_SVCS_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/mm-rt-svcs)

################################################
# List the public interfaces and headers to be
# exposed to Master Minion Runtime Sevrices
################################################

#Listing of header only public interfaces
set(MM_RT_SVCS_HDRS
    include/etsoc/common/common_defs.h
    include/etsoc/common/log_common.h
    include/etsoc/isa/atomic.h
    include/etsoc/isa/atomic-impl.h
    include/etsoc/isa/cacheops.h
    include/etsoc/isa/esr_defines.h
    include/etsoc/isa/etsoc_memory.h
    include/etsoc/isa/fcc.h
    include/etsoc/isa/flb.h
    include/etsoc/isa/hart.h
    include/etsoc/isa/hpm_counter.h
    include/etsoc/isa/io.h
    include/etsoc/isa/macros.h
    include/etsoc/isa/sync.h
    include/etsoc/isa/syscall.h
    include/etsoc/isa/riscv_encoding.h
    include/etsoc/isa/utils.h
)

#Listing of public headers that expose services provided by
#the MINION_BL (Minion Bootloader Library
set(MM_RT_SVCS_LIB_HDRS
    include/common/printf.h
    include/etsoc/drivers/serial/serial.h
    include/etsoc/drivers/pcie/pcie_int.h
    include/etsoc/drivers/pcie/pcie_device.h
    include/etsoc/drivers/pmu/pmu.h
    include/etsoc/isa/etsoc_rt_memory.h
    include/transports/circbuff/circbuff.h
    include/transports/vq/vq.h
    include/transports/mm_cm_iface/broadcast.h
    include/transports/mm_cm_iface/message_types.h
    include/transports/sp_mm_iface/sp_mm_comms_spec.h
    include/transports/sp_mm_iface/sp_mm_iface.h
    include/transports/sp_mm_iface/sp_mm_shared_config.h
)

##########################
#Create mm-rt-svcs library
##########################

#Listing of sources that implement services provided by
#the MM_RT_SVCS (Master Minion Runtime Services) Library
add_library(mm-rt-svcs STATIC
    src/common/printf.c
    src/etsoc/drivers/serial/serial.c
    src/etsoc/drivers/pcie/pcie_int.c
    src/etsoc/drivers/pmu/pmu.c
    src/etsoc/isa/etsoc_memory.c
    src/transports/circbuff/circbuff.c
    src/transports/vq/vq.c
    src/transports/mm_cm_iface/broadcast.c
    src/transports/sp_mm_iface/sp_mm_iface.c
)

set_target_properties(mm-rt-svcs PROPERTIES LINKER_LANGUAGE C)

target_link_libraries(mm-rt-svcs PUBLIC etsoc_hal::etsoc_hal)

target_include_directories(mm-rt-svcs
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${MM_RT_SVCS_INSTALL_PREFIX}/include>
)

target_compile_options(mm-rt-svcs
    PRIVATE
        -Wall
        -fno-strict-aliasing
        $<$<BOOL:${ENABLE_WARNINGS_AS_ERRORS}>:-Werror>
)

target_compile_definitions(mm-rt-svcs
    PUBLIC
        -DMM_RT=1
)

###################################################
#Install and export mmm-rt-svcs library and headers
###################################################

#This macro preserves the driectory structure as defined by the
#MM_RT_SVCS listing above
MACRO(InstallHdrsWithDirStruct HEADER_LIST)
    FOREACH(HEADER ${${HEADER_LIST}})
    STRING(REGEX MATCH "(.*)[/\]" DIR ${HEADER})
    INSTALL(FILES ${HEADER} DESTINATION ${MM_RT_SVCS_INSTALL_PREFIX}/${DIR})
    ENDFOREACH(HEADER)
ENDMACRO(InstallHdrsWithDirStruct)

InstallHdrsWithDirStruct(MM_RT_SVCS_HDRS)
InstallHdrsWithDirStruct(MM_RT_SVCS_LIB_HDRS)

install(
    TARGETS mm-rt-svcs
    EXPORT mm-rt-svcsTargets
    LIBRARY DESTINATION ${MM_RT_SVCS_INSTALL_PREFIX}/lib
    ARCHIVE DESTINATION ${MM_RT_SVCS_INSTALL_PREFIX}/lib
    INCLUDES DESTINATION ${MM_RT_SVCS_INSTALL_PREFIX}/include
)

#TODO: Could be improved and made more flexible by exporting a package
install(
    EXPORT mm-rt-svcsTargets
    NAMESPACE et-common-libs::
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/et-common-libs/mm-rt-svcs
    COMPONENT mm-rt-svcs
)

