#------------------------------------------------------------------------------
# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0
#------------------------------------------------------------------------------

# Common compile + link plumbing for any kernel ELF targeting our RISC-V CPU.
# These are CPU-level concerns shared by every backend (etsoc1, erbium,
# erbium-soc1sim) -- the SoC differences (linker script, crt0, peripheral
# surface, arch flags) are encoded in the per-platform wrappers below.
#
# The IEEE libm wraps patch a small subset of libm into et_libm (compiled
# with -mno-fdiv) because the HW lacks the fdiv family.  Those routines
# are typically off the critical path (NaN generation in non-happy paths)
# but need to link cleanly.
set(_RISCV_LIBM_WRAPPED_FUNCS
  "-Wl,--wrap=__ieee754_sqrtf \
   -Wl,--wrap=__ieee754_powf -Wl,--wrap=__ieee754_pow -Wl,--wrap=pow -Wl,--wrap=powf \
   -Wl,--wrap=__ieee754_atanhf -Wl,--wrap=__ieee754_atanh -Wl,--wrap=log1pf -Wl,--wrap=log1p \
   -Wl,--wrap=__ieee754_asinf -Wl,--wrap=__ieee754_asin -Wl,--wrap=asinf -Wl,--wrap=asin \
   -Wl,--wrap=__ieee754_acosf -Wl,--wrap=__ieee754_acos -Wl,--wrap=acos \
   -Wl,--wrap=__ieee754_acoshf -Wl,--wrap=__ieee754_acosh -Wl,--wrap=acosh -Wl,--wrap=acoshf \
   -Wl,--wrap=logf -Wl,--wrap=log -Wl,--wrap=__ieee754_logf -Wl,--wrap=__ieee754_log")

# Linker relaxation is disabled in clang (relaxed code for data refs becomes
# position-dependent instead of pc-relative [SW-17713]) and in gcc (program
# layout is not 100% stable, harming debugging).  A sysroot-relative path is
# also injected in clang to find gnu-libgcc.a.
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(_RISCV_EXTRA_LINKER_OPTIONS "-Wno-unused-command-line-argument -L =../lib/gcc/riscv64-unknown-elf/8.2.0/")
else()
  set(_RISCV_EXTRA_LINKER_OPTIONS "")
endif()

set(_RISCV_LINKER_BASE
  "-Wl,--no-relax -nostdlib -nostartfiles -Wl,--gc-sections -e _start \
   ${_RISCV_EXTRA_LINKER_OPTIONS} ${_RISCV_LIBM_WRAPPED_FUNCS} \
   -Wl,--start-group -lm -lgcc")


#
# add_riscv_executable -- platform-agnostic core.
# Builds a kernel ELF cmake target with the shared CPU-level compile/link flags
# (libm IEEE wraps, --no-relax, baremetal compile flags, post-build
# unimplemented-instruction check, cxx_std_17, ENABLE_EXPORTS for
# kernel-argument header sharing with host launchers).
#
# Required:
#   TARGET_NAME            CMake target name (typically "<kernel>.elf")
#   SOURCES                Source file list
#
# Optional:
#   LINKER_SCRIPT          Absolute or relative path to a custom -T script
#   RUNTIME_LIB            Library that supplies _start / crt0 / startup
#                          (e.g. etsoc_crt0, et-common-libs::erbium-umode)
#   BASE_ADDRESS_DEFSYM    Value for -Wl,--defsym=BASE_ADDRESS=...
#                          (etsoc1 production: 0; debug-companion: ADDRESS+0x1000)
#   EXTRA_ARCH_FLAGS       Per-platform compile flags (e.g. erbium's
#                          -march=rv64imfc -mabi=lp64f -mcmodel=medany)
#   EXTRA_LINK_FLAGS       Per-platform link flags (e.g. erbium's
#                          -Wl,--no-warn-rwx-segments)
#
function(add_riscv_executable TARGET_NAME)
  set(options)
  set(oneValueArgs LINKER_SCRIPT RUNTIME_LIB BASE_ADDRESS_DEFSYM)
  set(multiValueArgs SOURCES EXTRA_ARCH_FLAGS EXTRA_LINK_FLAGS)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  add_executable(${TARGET_NAME} ${ARG_SOURCES})

  # ENABLE_EXPORTS lets a host launcher depend on a kernel-arguments header
  # exported by the kernel.
  set_property(TARGET ${TARGET_NAME} PROPERTY ENABLE_EXPORTS 1)

  if (ARG_RUNTIME_LIB)
    target_link_libraries(${TARGET_NAME} PRIVATE ${ARG_RUNTIME_LIB})
  endif()

  target_compile_options(${TARGET_NAME} PRIVATE
    ${ARG_EXTRA_ARCH_FLAGS}
    -falign-functions=64 -fno-jump-tables -O3 -g3
    $<$<C_COMPILER_ID:GNU>:-Wstack-usage=4096>
    # baremetal & startup
    -fno-exceptions -fno-rtti -fno-unwind-tables
    -fno-use-cxa-atexit -fno-threadsafe-statics -ffreestanding
  )
  target_compile_features(${TARGET_NAME} PUBLIC cxx_std_17)

  set(_link_flags "${_RISCV_LINKER_BASE}")
  if (ARG_LINKER_SCRIPT)
    get_filename_component(_linker_script_abs "${ARG_LINKER_SCRIPT}" ABSOLUTE)
    string(APPEND _link_flags " -T ${_linker_script_abs}")
    set_target_properties(${TARGET_NAME} PROPERTIES LINK_DEPENDS "${_linker_script_abs}")
  endif()
  if (DEFINED ARG_BASE_ADDRESS_DEFSYM)
    string(APPEND _link_flags " -Wl,--defsym=BASE_ADDRESS=${ARG_BASE_ADDRESS_DEFSYM}")
  endif()
  foreach(_flag IN LISTS ARG_EXTRA_LINK_FLAGS)
    string(APPEND _link_flags " ${_flag}")
  endforeach()
  set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS "${_link_flags}")

  # Post-build: warn on any unimplemented-on-HW instructions that snuck in.
  add_custom_command(TARGET ${TARGET_NAME}
    POST_BUILD
    COMMAND ${GP_SDK_TOOLS_PATH}/scripts/check_unimplemented_instructions.sh ${TARGET_NAME}
  )
endfunction()


#
# Adds a riscv executable prepared for being executed as a kernel on ET-SoC-1.
# @param TARGET_NAME name of the cmake target to be created.
# @param TARGET_SOURCES_LIST space-separated list of source files
# Notes:
# - This will also create a secondary target `${TARGET_NAME}_dbg`.
#   The debug target is relinked using a runtime-predicted address and should be used with debuggers.
# - The created target is a standard cmake target, so compilation options and dependencies  can be
#   set with standard cmake functions (target_link_libraries, target_include_directories, etc.).
# - Requires ADDRESS to be defined (predicted device load address).
#
macro(add_etsoc_riscv_executable TARGET_NAME TARGET_SOURCES_LIST)
  if (NOT DEFINED ADDRESS)
    message(FATAL_ERROR "add_etsoc_riscv_executable(${TARGET_NAME}): ADDRESS is not set. "
            "Pass -DADDRESS=<predicted device load address> to cmake "
            "(e.g. -DADDRESS:STRING=0x8006335000).")
  endif()
  math(EXPR DEBUG_ADDRESS "${ADDRESS} + 0x1000" OUTPUT_FORMAT HEXADECIMAL)
  message(STATUS "Base address used to relink (debug) ELFs -> ${DEBUG_ADDRESS}")
  # merge parameter list
  set(TARGET_SOURCES ${TARGET_SOURCES_LIST} ${ARGN})

  # Production target: BASE_ADDRESS=0 (relocated at runtime by the
  # host runtime's relocateELF()).
  #
  # `-Wl,--emit-relocs` preserves the .rela.* sections in the linked
  # ELF.  esperanto-tools-libs/src/RuntimeImp.cpp:relocateSection()
  # walks SHT_RELA at load time and patches every R_RISCV_64 entry by
  # adding (deviceBuffer - elfBaseAddr) so that absolute 64-bit
  # pointers in .rodata / .data resolve to the runtime address.
  # Without this, things like the device_config singleton's
  # entryPoint_0 / entryPoint_1 function pointers stay at their
  # link-time addresses (0x40, 0x140, ...), and the kernel jumps to
  # absolute 0x40 on launch -> instruction access fault on every hart.
  # This flag was lost during the gp-sdk consolidation; was originally
  # added in commit nvxqwwsu ("gp-sdk/device: add --emit-relocs to
  # linker flags for runtime relocation").
  add_riscv_executable(${TARGET_NAME}
    SOURCES ${TARGET_SOURCES}
    LINKER_SCRIPT ${GP_SDK_LINKER_SCRIPT}
    RUNTIME_LIB et-common-libs::cm-umode
    BASE_ADDRESS_DEFSYM 0
    EXTRA_LINK_FLAGS -Wl,--emit-relocs
  )

  # Debug companion: identical sources, relinked at the runtime-predicted
  # address so symbols match what a debugger sees.
  set(DEBUG_TARGET "${TARGET_NAME}_dbg")
  add_riscv_executable(${DEBUG_TARGET}
    SOURCES ${TARGET_SOURCES}
    LINKER_SCRIPT ${GP_SDK_LINKER_SCRIPT}
    RUNTIME_LIB et-common-libs::cm-umode
    BASE_ADDRESS_DEFSYM ${DEBUG_ADDRESS}
  )
  # Mirror compile-time properties from the production target, since some
  # callers only set them on TARGET_NAME after the macro returns
  # (e.g. tests/CMakeLists.txt does target_link_libraries(${name} etsoc_crt0)
  # post-macro; the dbg target needs the same libs and the include
  # directories they transitively bring).
  target_link_libraries(${DEBUG_TARGET}        PRIVATE $<TARGET_PROPERTY:${TARGET_NAME},LINK_LIBRARIES>)
  target_include_directories(${DEBUG_TARGET}   PRIVATE $<TARGET_PROPERTY:${TARGET_NAME},INCLUDE_DIRECTORIES>)
  target_compile_options(${DEBUG_TARGET}       PRIVATE $<TARGET_PROPERTY:${TARGET_NAME},COMPILE_OPTIONS>)
  target_compile_definitions(${DEBUG_TARGET}   PRIVATE $<TARGET_PROPERTY:${TARGET_NAME},COMPILE_DEFINITIONS>)
endmacro(add_etsoc_riscv_executable)


#
# Adds a riscv executable prepared for being executed as an Erbium kernel.
# @param TARGET_NAME              name of the cmake target to be created
#                                 (conventionally "<kernel>.elf").
# @param SOURCES                  source file list (multi-value, named arg)
# @param BACKEND                  "erbium" (native MCU) or "erbium-soc1sim"
#                                 (Erbium kernels running on ET-SoC-1).
#                                 Picks the et-common-libs runtime + linker
#                                 script.  Defaults to "erbium-soc1sim".
# @param STACK_SIZE               optional per-hart stack size in bytes
#                                 (passed via -Wl,--defsym=STACK_SIZE=...).
# Notes:
# - Erbium kernels self-relocate at boot, so no _dbg companion target is
#   produced (unlike the etsoc1 variant).
# - Arch flags are rv64imfc / lp64f / medany; the freestanding compile
#   flags from the shared core apply.
#
function(add_erbium_riscv_executable TARGET_NAME)
  set(options)
  set(oneValueArgs BACKEND STACK_SIZE)
  set(multiValueArgs SOURCES)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (NOT ARG_BACKEND)
    set(ARG_BACKEND "erbium-soc1sim")
  endif()

  if (ARG_BACKEND STREQUAL "erbium")
    set(_runtime_lib erbium_crt0)
  elseif (ARG_BACKEND STREQUAL "erbium-soc1sim")
    set(_runtime_lib erbium_soc1sim_crt0)
  else()
    message(FATAL_ERROR "add_erbium_riscv_executable(${TARGET_NAME}): "
            "BACKEND must be 'erbium' or 'erbium-soc1sim' (got '${ARG_BACKEND}').")
  endif()

  # Pick up the linker script that the selected backend publishes as a
  # target property.
  get_target_property(_linker_script ${_runtime_lib} ERBIUM_LINKER_SCRIPT)
  if (NOT _linker_script)
    message(FATAL_ERROR "add_erbium_riscv_executable(${TARGET_NAME}): "
            "${_runtime_lib} does not publish ERBIUM_LINKER_SCRIPT.")
  endif()

  # _RISCV_LINKER_BASE hardcodes `-e _start` (an etsoc1 convention -- their
  # firmware delivers in U-mode at `_start`, no M-mode boot).  Erbium's
  # linker scripts declare `ENTRY(_boot)`: native erbium needs `_boot` for
  # the M-mode -> U-mode drop (sp setup, mret), and erbium-soc1sim's
  # `_boot` is a `j _start` trampoline.  Override -e here so the late -e
  # wins on the link line; this also adds `_boot` to the undefined-symbol
  # list, which forces ld to pull boot.o out of the static archive
  # (`ENTRY(_boot)` alone does NOT trigger archive resolution, so without
  # this the kernel silently links with crt.S's `_start` only and the ELF
  # e_entry falls back to `_start` -- fatal on native erbium because main
  # then runs with sp = 0).
  set(_extra_link_flags "-Wl,--no-warn-rwx-segments" "-Wl,-e,_boot")
  if (DEFINED ARG_STACK_SIZE)
    list(APPEND _extra_link_flags "-Wl,--defsym=STACK_SIZE=${ARG_STACK_SIZE}")
  endif()

  add_riscv_executable(${TARGET_NAME}
    SOURCES        ${ARG_SOURCES}
    LINKER_SCRIPT  ${_linker_script}
    RUNTIME_LIB    ${_runtime_lib}
    EXTRA_ARCH_FLAGS
      -march=rv64imfc -mabi=lp64f -mcmodel=medany
      -nostdlib -fno-zero-initialized-in-bss
      -ffunction-sections -fdata-sections
    EXTRA_LINK_FLAGS ${_extra_link_flags}
  )

  # Erbium kernels benefit from IPO/-flto (matches the policy in the prior
  # erbium-examples build).
  set_target_properties(${TARGET_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endfunction()
