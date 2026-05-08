####################################
# Erbium Minion User Mode (soc1sim)
####################################
# Header-only "API surface" for the soc1sim backend: ISA headers and
# the boot/crt helper macros, all shadow-installed under
# `<erbium/...>` so a kernel's source can stay backend-agnostic. The
# default boot/crt sources and linker script live in gp-sdk
# (`device/sdk/lib/erbium-soc1sim/`); see gp-sdk's
# `erbium_soc1sim_crt0` STATIC library for the do-by-example glue.
#
# Source layout mirrors the other et-common-libs ISA packages:
#
#     include/erbium-soc1sim/isa/*.h
#     include/erbium-soc1sim/{boot,crt}.h
#
# The `erbium/` piece in the consumer-visible `#include <erbium/...>`
# path is injected at build-stage and at install time.

set(ERBIUM_SOC1SIM_UMODE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/erbium-soc1sim-umode)

################################################
# List the public interfaces and headers to be
# exposed to Erbium Minion User Mode (soc1sim) here
################################################

# Listing of header only public interfaces
set(ERBIUM_SOC1SIM_UMODE_HDRS
    # Erbium ISA headers (soc1sim-backed)
    include/erbium-soc1sim/isa/atomic.h
    include/erbium-soc1sim/isa/atomic-impl.h
    include/erbium-soc1sim/isa/barriers.h
    include/erbium-soc1sim/isa/cacheops.h
    include/erbium-soc1sim/isa/cacheops-umode.h
    include/erbium-soc1sim/isa/esr_defines.h
    include/erbium-soc1sim/isa/fcc.h
    include/erbium-soc1sim/isa/flb.h
    include/erbium-soc1sim/isa/hart.h
    include/erbium-soc1sim/isa/layout.h
    include/erbium-soc1sim/isa/sync.h
    include/erbium-soc1sim/isa/syscall.h
    include/erbium-soc1sim/isa/tensors.h
    include/erbium-soc1sim/isa/utils.h
    # Runtime helpers (boot/crt building blocks; shadow <erbium/...>)
    include/erbium-soc1sim/boot.h
    include/erbium-soc1sim/crt.h
)

# Listing of public headers that expose services provided by
# the Erbium UMODE (soc1sim) Library
set(ERBIUM_SOC1SIM_UMODE_LIB_HDRS
)

####################################
# Create erbium-soc1sim interface
####################################
# Stage headers under <build>/erbium-soc1sim-staged-include/erbium/
# so BUILD_INTERFACE consumers see the same `<erbium/...>` path as
# INSTALL_INTERFACE consumers. The `erbium/` piece exists only in
# the staged/installed trees — the source tree keeps the symmetric
# include/erbium-soc1sim/ layout.
set(ERBIUM_SOC1SIM_UMODE_STAGED_INCLUDE
    ${CMAKE_CURRENT_BINARY_DIR}/erbium-soc1sim-staged-include)
file(REMOVE_RECURSE ${ERBIUM_SOC1SIM_UMODE_STAGED_INCLUDE})
file(MAKE_DIRECTORY ${ERBIUM_SOC1SIM_UMODE_STAGED_INCLUDE}/erbium)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/include/erbium-soc1sim/isa
          ${CMAKE_CURRENT_SOURCE_DIR}/include/erbium-soc1sim/boot.h
          ${CMAKE_CURRENT_SOURCE_DIR}/include/erbium-soc1sim/crt.h
     DESTINATION ${ERBIUM_SOC1SIM_UMODE_STAGED_INCLUDE}/erbium/)

# INTERFACE library: ships only headers. The default boot/crt and
# linker script live in gp-sdk; downstream consumers link gp-sdk's
# `erbium_soc1sim_crt0` (which transitively pulls this target for
# headers).
add_library(erbium-soc1sim INTERFACE)
add_library(et-common-libs::erbium-soc1sim ALIAS erbium-soc1sim)
target_include_directories(erbium-soc1sim
    INTERFACE
        $<BUILD_INTERFACE:${ERBIUM_SOC1SIM_UMODE_STAGED_INCLUDE}>
        $<INSTALL_INTERFACE:${ERBIUM_SOC1SIM_UMODE_INSTALL_PREFIX}/include>
)
target_link_libraries(erbium-soc1sim INTERFACE etsoc_hal::etsoc_hal)

# Install headers under `<prefix>/include/erbium/...` to match the
# consumer's `#include <erbium/...>` path. Strip the leading
# `include/erbium-soc1sim/` from each source path and prepend
# `erbium/` at install time.
macro(InstallErbiumSoc1simHdrsWithDirStruct HEADER_LIST)
    foreach(HEADER ${${HEADER_LIST}})
        string(REGEX REPLACE "^include/erbium-soc1sim/" "" REL_HEADER ${HEADER})
        get_filename_component(REL_DIR ${REL_HEADER} DIRECTORY)
        install(FILES ${HEADER}
            DESTINATION ${ERBIUM_SOC1SIM_UMODE_INSTALL_PREFIX}/include/erbium/${REL_DIR})
    endforeach(HEADER)
endmacro(InstallErbiumSoc1simHdrsWithDirStruct)

InstallErbiumSoc1simHdrsWithDirStruct(ERBIUM_SOC1SIM_UMODE_HDRS)
InstallErbiumSoc1simHdrsWithDirStruct(ERBIUM_SOC1SIM_UMODE_LIB_HDRS)

##########################################################
# Install and export erbium-soc1sim library and headers
##########################################################

install(
    TARGETS erbium-soc1sim
    EXPORT erbium-soc1simTargets
    INCLUDES DESTINATION ${ERBIUM_SOC1SIM_UMODE_INSTALL_PREFIX}/include
)

install(
    EXPORT erbium-soc1simTargets
    NAMESPACE et-common-libs::
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/et-common-libs/erbium-soc1sim
    COMPONENT erbium-soc1sim
)
