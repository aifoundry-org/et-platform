########################
# SP Boot Loader 2 Lib
########################

#SP Boot Loader requires etsoc_hal library
find_package(etsoc_hal REQUIRED)

#Install prefix for SP Bootloader 2 library
set(SP_BL2_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/sp-bl2)

################################################
# List the public interfaces and headers to be
# exposed to SP Boot Loader
################################################

#Listing of header only public interfaces
set(SP_BL2_HDRS
    include/etsoc/common/common_defs.h
    include/etsoc/common/log_common.h
    include/etsoc/drivers/pcie/pcie_int.h
    include/etsoc/drivers/pmu/pmu.h
    include/etsoc/isa/atomic.h
    include/etsoc/isa/atomic-impl.h
    include/etsoc/isa/etsoc_memory.h
    include/etsoc/isa/io.h
    include/etsoc/isa/cacheops.h
    include/etsoc/isa/cacheops_common.h
    include/etsoc/isa/esr_defines.h
    include/etsoc/isa/fcc.h
    include/etsoc/isa/utils.h
    include/transports/vq/vq.h
    include/transports/circbuff/circbuff.h
    include/transports/sp_mm_iface/sp_mm_comms_spec.h
    include/transports/mm_cm_iface/message_types.h
    include/transports/sp_mm_iface/sp_mm_iface.h
    include/transports/sp_mm_iface/sp_mm_shared_config.h
    include/system/layout.h
    include/system/etsoc_ddr_region_map.h
)

#Listing of public headers that expose services provided by
#the SP_BL2 (SP Bootloader 2 Library)
set(SP_BL2_LIB_HDRS
    include/etsoc/drivers/serial/serial.h
    include/etsoc/isa/etsoc_rt_memory.h
)

#########################
#Create SP-BL 2 library
#########################

#Listing of sources that implement services provided by
#the SP_BL2 (SP Bootloader 2 Library)
add_library(sp-bl2 STATIC
    src/etsoc/isa/etsoc_memory.c
    src/etsoc/drivers/pcie/pcie_int.c
    src/transports/circbuff/circbuff.c
    src/transports/vq/vq.c
    src/transports/sp_mm_iface/sp_mm_iface.c
)
add_library(et-common-libs::sp-bl2 ALIAS sp-bl2)

target_compile_definitions(sp-bl2
    PUBLIC
        -DSP_RT=1
        -DSERVICE_PROCESSOR_BL2=1
)

set_target_properties(sp-bl2 PROPERTIES LINKER_LANGUAGE C)

target_link_libraries(sp-bl2
    PUBLIC
        etsoc_hal::etsoc_hal
        esperantoTrace::et_trace
)

target_include_directories(sp-bl2
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${SP_BL2_INSTALL_PREFIX}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/esperanto-fw>
)

target_compile_features(sp-bl2 PUBLIC c_std_11)

target_compile_options(sp-bl2
    PRIVATE
        -Wall
        -fno-strict-aliasing
        $<$<BOOL:${ENABLE_WARNINGS_AS_ERRORS}>:-Werror>
)

#################################################
#Install and export sp-bl2 library and headers
#################################################

#This macro preserves the driectory structure as defined by the
#SP_BL2 (SP Bootloader 2 Library)listing above
macro(InstallHdrsWithDirStruct HEADER_LIST)
    foreach(HEADER ${${HEADER_LIST}})
    string(REGEX MATCH "(.*)[/\]" DIR ${HEADER})
    install(FILES ${HEADER} DESTINATION ${SP_BL2_INSTALL_PREFIX}/${DIR})
    endforeach(HEADER)
endmacro(InstallHdrsWithDirStruct)

InstallHdrsWithDirStruct(SP_BL2_HDRS)
InstallHdrsWithDirStruct(SP_BL2_LIB_HDRS)

install(
    TARGETS sp-bl2
    EXPORT sp-bl2Targets
    LIBRARY DESTINATION ${SP_BL2_INSTALL_PREFIX}/lib
    ARCHIVE DESTINATION ${SP_BL2_INSTALL_PREFIX}/lib
    INCLUDES DESTINATION ${SP_BL2_INSTALL_PREFIX}/include
)

#TODO: Could be improved and made more flexible by exporting a package
install(
    EXPORT sp-bl2Targets
    NAMESPACE et-common-libs::
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/et-common-libs/sp-bl2
)

