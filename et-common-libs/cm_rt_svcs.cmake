##########################################
# Compute Minion Run Time (CM-RT) Services
##########################################

#CM-RT requires etsoc_hal library
find_package(etsoc_hal REQUIRED)

#Install prefix for CM-RT library
set(CM_RT_SVCS_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/cm-rt-svcs)

################################################
# List the public interfaces and headers to be
# exposed to Compute Minion Runtime Sevrices
################################################

#Listing of header only public interfaces
set(CM_RT_SVCS_HDRS
    include/etsoc/common/common_defs.h
    include/etsoc/common/log_common.h
    include/etsoc/isa/atomic.h
    include/etsoc/isa/atomic-impl.h
    include/etsoc/isa/esr_defines.h
    include/etsoc/isa/fcc.h
    include/etsoc/isa/flb.h
    include/etsoc/isa/hart.h
    include/etsoc/isa/io.h
    include/etsoc/isa/etsoc_memory.h
    include/etsoc/isa/macros.h
    include/etsoc/isa/cacheops.h
    include/etsoc/isa/sync.h
    include/etsoc/isa/syscall.h
    include/etsoc/isa/riscv_encoding.h
    include/etsoc/isa/utils.h
    include/system/abi.h
    include/system/etsoc_ddr_region_map.h
    include/system/layout.h
)

#Listing of public headers that expose services provided by
#the CM_RT_SVCS (Compute Minion Runtime Library)
set(CM_RT_SVCS_LIB_HDRS
    include/common/printf.h
    include/transports/circbuff/circbuff.h
    include/etsoc/drivers/pmu/pmu.h
    include/etsoc/drivers/serial/serial.h
    include/transports/mm_cm_iface/message_types.h
)

##########################
#Create cm-rt-svcs library
##########################

#Listing of sources that implement services provided by
#the CM_RT_SVCS (Compute Minion Runtime Services) Library
add_library(cm-rt-svcs STATIC
    src/common/printf.c
    src/etsoc/isa/etsoc_memory.c
    src/etsoc/drivers/pmu/pmu.c
    src/etsoc/drivers/serial/serial.c
    src/transports/circbuff/circbuff.c
)

set_target_properties(cm-rt-svcs PROPERTIES LINKER_LANGUAGE C)

target_link_libraries(cm-rt-svcs
    PUBLIC
        etsoc_hal::etsoc_hal
        esperantoTrace::et_trace
)

target_include_directories(cm-rt-svcs
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CM_RT_SVCS_INSTALL_PREFIX}/include>
)

target_compile_features(cm-rt-svcs PUBLIC c_std_11)

target_compile_options(cm-rt-svcs
    PRIVATE
        -Wall
        -fno-strict-aliasing
        -fno-zero-initialized-in-bss -fdata-sections
        $<$<BOOL:${ENABLE_WARNINGS_AS_ERRORS}>:-Werror>
)


###################################################
#Install and export cm-rt-svcs library and headers
###################################################

#This macro preserves the driectory structure as defined by the
#CM_RT_SVCS listing above
MACRO(InstallHdrsWithDirStruct HEADER_LIST)
    FOREACH(HEADER ${${HEADER_LIST}})
    STRING(REGEX MATCH "(.*)[/\]" DIR ${HEADER})
    INSTALL(FILES ${HEADER} DESTINATION ${CM_RT_SVCS_INSTALL_PREFIX}/${DIR})
    ENDFOREACH(HEADER)
ENDMACRO(InstallHdrsWithDirStruct)

InstallHdrsWithDirStruct(CM_RT_SVCS_HDRS)
InstallHdrsWithDirStruct(CM_RT_SVCS_LIB_HDRS)

install(
    TARGETS cm-rt-svcs
    EXPORT cm-rt-svcsTargets
    LIBRARY DESTINATION ${CM_RT_SVCS_INSTALL_PREFIX}/lib
    ARCHIVE DESTINATION ${CM_RT_SVCS_INSTALL_PREFIX}/lib
    INCLUDES DESTINATION ${CM_RT_SVCS_INSTALL_PREFIX}/include
)

#TODO: Could be improved and made more flexible by exporting a package
install(
    EXPORT cm-rt-svcsTargets
    NAMESPACE et-common-libs::
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/et-common-libs/cm-rt-svcs
    COMPONENT cm-rt-svcs
)
