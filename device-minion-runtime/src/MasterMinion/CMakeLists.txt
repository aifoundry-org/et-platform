#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

# Add the device_api submodule as an external project. The code-generator code needs to always
# compile for the host. CMake does not support having different targets use
# different toolchains in the same build. For simplicity submodules that are self
# contained build as external projects, otherwise a two-step process would be needded.

set(LINKER_SCRIPT src/sections.ld)
set(LINKER_SCRIPT_DEPENDENCY ${SHARED_INC_DIR}/layout.ld)

include_directories(${SHARED_INC_DIRS})

if(EsperantoCryptoAPI_FOUND)
  include_directories(${CRYPTO_HEADERS})
else()
  include_directories(${CRYPTO_HEADERS}/esperanto/crypto-api)
endif(EsperantoCryptoAPI_FOUND)

add_riscv_executable(MasterMinion 
    src/crt.S
    src/trap_handler.S
    src/exception.c
    src/main.c
    src/build_configuration.c
    src/config/dir_regs.c
    src/dispatcher/dispatcher.c
    src/workers/sqw.c
    src/workers/dmaw.c
    src/workers/kw.c
    src/workers/cw.c
    src/drivers/console.c
    src/drivers/pcie_dma.c
    src/drivers/plic.c
    src/drivers/pu_timers.c
    src/services/log.c
    src/services/host_iface.c
    src/services/host_cmd_hdlr.c
    src/services/cm_iface.c
    src/services/sp_iface.c
    src/services/sp_cmd_hdlr.c
    src/services/sw_timer.c
    src/services/trace.c
    ${SHARED_SRC_DIR}/etsoc_memory.c
    ${SHARED_SRC_DIR}/serial.c
    ${SHARED_SRC_DIR}/printf.c
    ${SHARED_SRC_DIR}/putchar.c
    ${SHARED_SRC_DIR}/print_exception.c
    ${SHARED_SRC_DIR}/pcie_int.c
    ${SHARED_SRC_DIR}/broadcast.c
    ${SHARED_SRC_DIR}/circbuff.c
    ${SHARED_SRC_DIR}/vq.c
)

target_compile_definitions(MasterMinion.elf PUBLIC -DMASTER_MINION=1)
target_include_directories(MasterMinion.elf
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${ESPERANTO_DEVICE_API_INCLUDE_DIR}
)
target_link_libraries(MasterMinion.elf PUBLIC deviceFirmware::device_trace)

install(TARGETS MasterMinion.elf
    EXPORT EsperantoDeviceMinionRuntimeTargets
    RUNTIME DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/MasterMinion
    COMPONENT master-minion
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/MasterMinion.lst
              ${CMAKE_CURRENT_BINARY_DIR}/MasterMinion.map
    DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/MasterMinion
    COMPONENT master-minion
)
