#################################
# Service Processor Boot Loader 1
#################################

#Minion Boot Loader requires etsoc_hal library
find_package(etsoc_hal REQUIRED)

#Install prefix for Minion Bootloader 1 library
set(SP_BL1_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/sp-bl1)

################################################
# List the public interfaces and headers to be
# exposed to SP Boot Loader 1
################################################
#Listing of header only public interfaces
set(SP_BL1_HDRS
    include/etsoc/drivers/pmu/pmu.h
    include/etsoc/isa/io.h
    include/etsoc/isa/atomic.h
    include/etsoc/isa/atomic-impl.h
    include/etsoc/isa/etsoc_memory.h
    include/etsoc/isa/esr_defines.h
    include/etsoc/isa/macros.h
    include/etsoc/isa/fcc.h
    include/etsoc/isa/flb.h
    include/etsoc/isa/hart.h
    include/etsoc/isa/sync.h
    include/etsoc/isa/cacheops.h
    include/etsoc/isa/cacheops_common.h
    include/etsoc/isa/riscv_encoding.h
    include/etsoc/isa/utils.h
    include/system/layout.h
    include/system/etsoc_ddr_region_map.h
)

#Listing of public headers that expose services provided by
#the SP_BL1 (Minion Bootloader Library
set(SP_BL1_LIB_HDRS
    include/etsoc/drivers/serial/serial.h
)

#########################
#Create sp-bl1 library
#########################

#Listing of sources that implement services provided by
#the SP_BL1 (Minion Bootloader) Library
add_library(sp-bl1 STATIC
    src/etsoc/drivers/serial/serial.c
)

set_target_properties(sp-bl1 PROPERTIES LINKER_LANGUAGE C)

target_link_libraries(sp-bl1 PUBLIC etsoc_hal::etsoc_hal)

target_include_directories(sp-bl1
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${SP_BL1_INSTALL_PREFIX}/include>
)

#################################################
#Install and export sp-bl1 library and headers
#################################################

#This macro preserves the driectory structure as defined by the
#SP_BL1 listing above
macro(InstallHdrsWithDirStruct HEADER_LIST)
    foreach(HEADER ${${HEADER_LIST}})
    string(REGEX MATCH "(.*)[/\]" DIR ${HEADER})
    install(FILES ${HEADER} DESTINATION ${SP_BL1_INSTALL_PREFIX}/${DIR})
    endforeach(HEADER)
endmacro(InstallHdrsWithDirStruct)

InstallHdrsWithDirStruct(SP_BL1_HDRS)
InstallHdrsWithDirStruct(SP_BL1_LIB_HDRS)

install(
    TARGETS sp-bl1
    EXPORT sp-bl1Targets
    LIBRARY DESTINATION ${SP_BL1_INSTALL_PREFIX}/lib
    ARCHIVE DESTINATION ${SP_BL1_INSTALL_PREFIX}/lib
    INCLUDES DESTINATION ${SP_BL1_INSTALL_PREFIX}/include
)

#TODO: Could be improved and made more flexible by exporting a package
install(
    EXPORT sp-bl1Targets
    NAMESPACE et-common-libs::
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/et-common-libs/sp-bl1
)
