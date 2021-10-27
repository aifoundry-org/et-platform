########################
# Minion Boot Loader
########################

#Minion Boot Loader requires etsoc_hal library
find_package(etsoc_hal REQUIRED)

#Install prefix for Minion Bootloader library
set(MINION_BL_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/minion-bl)

################################################
# List the public interfaces and headers to be
# exposed to Minion Boot Loader
################################################

#Listing of header only public interfaces
set(MINION_BL_HDRS
    include/etsoc/drivers/pmu/pmu.h
    include/etsoc/isa/atomic.h
    include/etsoc/isa/atomic-impl.h
    include/etsoc/isa/esr_defines.h
    include/etsoc/isa/macros.h
    include/etsoc/isa/fcc.h
    include/etsoc/isa/flb.h
    include/etsoc/isa/hart.h
    include/etsoc/isa/sync.h
    include/etsoc/isa/cacheops.h
    include/etsoc/isa/syscall.h
    include/etsoc/isa/riscv_encoding.h
    include/etsoc/isa/utils.h
    include/system/etsoc_ddr_region_map.h
    include/system/layout.h
)

#Listing of public headers that expose services provided by
#the MINION_BL (Minion Bootloader Library
set(MINION_BL_LIB_HDRS
    include/transports/mm_cm_iface/broadcast.h
)

#########################
#Create minion-bl library
#########################

#Listing of sources that implement services provided by
#the MINION_BL (Minion Bootloader) Library
add_library(minion-bl STATIC
    src/etsoc/drivers/pmu/pmu.c
    src/transports/mm_cm_iface/broadcast.c
)

set_target_properties(minion-bl PROPERTIES LINKER_LANGUAGE C)

target_link_libraries(minion-bl PUBLIC etsoc_hal::etsoc_hal)

target_include_directories(minion-bl
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${MINION_BL_INSTALL_PREFIX}/include>
)

target_compile_options(minion-bl
    PRIVATE
        -Wall
        -fno-strict-aliasing
        $<$<BOOL:${ENABLE_WARNINGS_AS_ERRORS}>:-Werror>
)


#################################################
#Install and export minion-bl library and headers
#################################################

#This macro preserves the driectory structure as defined by the
#MINION_BL listing above
MACRO(InstallHdrsWithDirStruct HEADER_LIST)
    FOREACH(HEADER ${${HEADER_LIST}})
    STRING(REGEX MATCH "(.*)[/\]" DIR ${HEADER})
    INSTALL(FILES ${HEADER} DESTINATION ${MINION_BL_INSTALL_PREFIX}/${DIR})
    ENDFOREACH(HEADER)
ENDMACRO(InstallHdrsWithDirStruct)

InstallHdrsWithDirStruct(MINION_BL_HDRS)
InstallHdrsWithDirStruct(MINION_BL_LIB_HDRS)

install(
    TARGETS minion-bl
    EXPORT minion-blTargets
    LIBRARY DESTINATION ${MINION_BL_INSTALL_PREFIX}/lib
    ARCHIVE DESTINATION ${MINION_BL_INSTALL_PREFIX}/lib
    INCLUDES DESTINATION ${MINION_BL_INSTALL_PREFIX}/include
)

#TODO: Could be improved and made more flexible by exporting a package
install(
    EXPORT minion-blTargets
    NAMESPACE et-common-libs::
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/et-common-libs/minion-bl
    COMPONENT minion-bl
)

