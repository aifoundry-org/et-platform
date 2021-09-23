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
    #Floating point operations
    include/float/Float16.h
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
    src/etsoc/hal/pmu.c
    src/common/utils.c
    src/trace/trace_umode.c
    src/common/printf.c
    #TODO:others to come ..
)
add_library(et-common-libs::cm-umode ALIAS cm-umode)

target_link_libraries(cm-umode PUBLIC esperantoTrace::et_trace)
set_target_properties(cm-umode PROPERTIES LINKER_LANGUAGE C)

target_include_directories(cm-umode
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CM_UMODE_INSTALL_PREFIX}/include>
)

target_compile_options(cm-umode
    PRIVATE
        -Wall
        $<$<BOOL:${ENABLE_WARNINGS_AS_ERRORS}>:-Werror>
)

#This macro preserves the driectory structure as defined by the
#CM UMODE listing above
MACRO(InstallHdrsWithDirStruct HEADER_LIST)
    FOREACH(HEADER ${${HEADER_LIST}})
    STRING(REGEX MATCH "(.*)[/\]" DIR ${HEADER})
    INSTALL(FILES ${HEADER} DESTINATION ${CM_UMODE_INSTALL_PREFIX}/${DIR})
    ENDFOREACH(HEADER)
ENDMACRO(InstallHdrsWithDirStruct)

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

