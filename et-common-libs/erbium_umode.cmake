############################
# Erbium Minion User Mode
############################
# Header-only "API surface" for erbium U-mode kernels: ISA headers
# and the boot/crt helper macros under <erbium/{boot,crt}.h>. The
# default boot/crt sources and linker script live in gp-sdk
# (`device/sdk/lib/erbium/`); see gp-sdk's `erbium_crt0` STATIC
# library for the do-by-example glue that links into a kernel ELF.

set(ERBIUM_UMODE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/erbium-umode)

################################################
# List the public interfaces and headers to be
# exposed to Erbium Minion User Mode here
################################################

# Listing of header only public interfaces
set(ERBIUM_UMODE_HDRS
    # Erbium ISA headers
    include/erbium/isa/atomic.h
    include/erbium/isa/atomic-impl.h
    include/erbium/isa/barriers.h
    include/erbium/isa/cacheops.h
    include/erbium/isa/cacheops-umode.h
    include/erbium/isa/esr_defines.h
    include/erbium/isa/fcc.h
    include/erbium/isa/flb.h
    include/erbium/isa/hart.h
    include/erbium/isa/layout.h
    include/erbium/isa/sync.h
    include/erbium/isa/tensors.h
    include/erbium/isa/utils.h
    # Erbium runtime helpers (boot/crt building blocks)
    include/erbium/boot.h
    include/erbium/crt.h
)

# Listing of public headers that expose services provided by
# the Erbium UMODE Library
set(ERBIUM_UMODE_LIB_HDRS
)

############################
# Create erbium-umode interface
############################
# INTERFACE library: ships only headers + transitive HAL link. The
# default boot/crt and linker script that pair with these headers
# live in gp-sdk; downstream consumers link gp-sdk's `erbium_crt0`
# (which transitively pulls this target for headers).
add_library(erbium-umode INTERFACE)
add_library(et-common-libs::erbium-umode ALIAS erbium-umode)
target_include_directories(erbium-umode
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${ERBIUM_UMODE_INSTALL_PREFIX}/include>
)
target_link_libraries(erbium-umode INTERFACE erbium_hal::erbium_hal)

# This macro preserves the directory structure as defined by the
# ERBIUM UMODE listing above
macro(InstallErbiumHdrsWithDirStruct HEADER_LIST)
    foreach(HEADER ${${HEADER_LIST}})
        string(REGEX MATCH "(.*)[/\]" DIR ${HEADER})
        install(FILES ${HEADER} DESTINATION ${ERBIUM_UMODE_INSTALL_PREFIX}/${DIR})
    endforeach(HEADER)
endmacro(InstallErbiumHdrsWithDirStruct)

InstallErbiumHdrsWithDirStruct(ERBIUM_UMODE_HDRS)
InstallErbiumHdrsWithDirStruct(ERBIUM_UMODE_LIB_HDRS)

####################################################
# Install and export erbium-umode library and headers
####################################################

install(
    TARGETS erbium-umode
    EXPORT erbium-umodeTargets
    INCLUDES DESTINATION ${ERBIUM_UMODE_INSTALL_PREFIX}/include
)

install(
    EXPORT erbium-umodeTargets
    NAMESPACE et-common-libs::
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/et-common-libs/erbium-umode
    COMPONENT erbium-umode
)
