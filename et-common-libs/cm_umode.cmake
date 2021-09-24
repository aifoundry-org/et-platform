########################
#Compute Minion User Mode
########################

set(CM_UMODE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/cm-umode)

################################################
# List the public interfaces and headers to be
#exposed to Compute Minion User Mode here
################################################

#Listing of header only public interfaces
set(CM_UMODE_HDRS
    # ETSOC ISA headers
    include/common/printf.h
    include/etsoc/isa/atomic.h
    include/etsoc/isa/atomic-impl.h
    include/etsoc/isa/barriers.h
    include/etsoc/isa/cacheops-umode.h
    include/etsoc/isa/esr_defines.h
    include/etsoc/isa/fcc.h
    include/etsoc/isa/flb.h
    include/etsoc/isa/hart.h
    include/etsoc/isa/syscall.h
    include/etsoc/isa/tensors.h
    include/etsoc/isa/utils.h
    include/common/printf.h
    # Trace Header
    include/trace/trace_umode.h
    #TODO:others to come ..
)

#Listing of public headers that expose services provided by
#the CM UMODE (Compute Minion User Mode) Library
set(CM_UMODE_LIB_HDRS
    include/etsoc/hal/pmu.h
    include/etsoc/common/utils.h
    #TODO:others to come ..
)

########################
#Create cm-umode library
########################
#Listing of sources that implement services provided by
#the CM UMODE (Compute Minion User Mode) Library
add_library(cm-umode STATIC
    src/common/printf.c
    src/etsoc/common/utils.c
    src/etsoc/drivers/pmu/pmu.c
    src/trace/trace_umode.c
    src/common/printf_dummy.c
    src/libc_stub/stdlib.c
    src/libc_stub/string.c
)
add_library(et-common-libs::cm-umode ALIAS cm-umode)
target_include_directories(cm-umode
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CM_UMODE_INSTALL_PREFIX}/include>
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/libc_stub
)
target_compile_options(cm-umode
    PRIVATE
        -Wall
        $<$<BOOL:${ENABLE_WARNINGS_AS_ERRORS}>:-Werror>
)
target_link_libraries(cm-umode
    PUBLIC
        esperantoTrace::et_trace
)

#This macro preserves the driectory structure as defined by the
#CM UMODE listing above
macro(InstallHdrsWithDirStruct HEADER_LIST)
    foreach(HEADER ${${HEADER_LIST}})
        string(REGEX MATCH "(.*)[/\]" DIR ${HEADER})
        install(FILES ${HEADER} DESTINATION ${CM_UMODE_INSTALL_PREFIX}/${DIR})
    endforeach(HEADER)
endmacro(InstallHdrsWithDirStruct)

InstallHdrsWithDirStruct(CM_UMODE_HDRS)
InstallHdrsWithDirStruct(CM_UMODE_LIB_HDRS)

################################################
#Install and export cm-umode library and headers
################################################

install(
    TARGETS cm-umode
    EXPORT cm-umodeTargets
    LIBRARY DESTINATION ${CM_UMODE_INSTALL_PREFIX}/lib
    ARCHIVE DESTINATION ${CM_UMODE_INSTALL_PREFIX}/lib
    INCLUDES DESTINATION ${CM_UMODE_INSTALL_PREFIX}/include
)

#TODO: Could be improved and made more flexible by exporting a package
install(
    EXPORT cm-umodeTargets
    NAMESPACE et-common-libs::
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/et-common-libs/cm-umode
    COMPONENT cm-umode
)

