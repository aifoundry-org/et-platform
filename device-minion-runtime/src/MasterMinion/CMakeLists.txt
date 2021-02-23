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
include(ExternalProject)

set(SOURCES
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
    src/services/log.c
    src/services/host_iface.c
    src/services/host_cmd_hdlr.c
    src/services/mm_to_cm_iface.c
    src/services/sp_iface.c
    src/services/sp_cmd_hdlr.c
    ${SHARED_SRC_DIR}/serial.c
    ${SHARED_SRC_DIR}/printf.c
    ${SHARED_SRC_DIR}/putchar.c
    ${SHARED_SRC_DIR}/cm_to_mm_iface.c
    ${SHARED_SRC_DIR}/print_exception.c
    ${SHARED_SRC_DIR}/pcie_int.c
    ${SHARED_SRC_DIR}/broadcast.c
    ${SHARED_SRC_DIR}/common.c
    ${SHARED_SRC_DIR}/circbuff.c
    ${SHARED_SRC_DIR}/vq.c
    )

set(TARGET_NAME MasterMinion)
set(LINKER_SCRIPT src/sections.ld)
set(LINKER_SCRIPT_DEPENDENCY ${SHARED_INC_DIR}/layout.ld)

include_directories(include)
include_directories(${ESPERANTO_DEVICE_API_INCLUDE_DIR})
include_directories(${SHARED_INC_DIRS})

if(EsperantoCryptoAPI_FOUND)
  include_directories(${CRYPTO_HEADERS})
else()
  include_directories(${CRYPTO_HEADERS}/esperanto/crypto-api)
endif(EsperantoCryptoAPI_FOUND)

add_riscv_executable(${TARGET_NAME} ${SOURCES})

target_compile_definitions(${TARGET_NAME}.elf PUBLIC -DMASTER_MINION=1)

target_link_libraries(${TARGET_NAME}.elf device-mrt-trace)

install(TARGETS ${TARGET_NAME}.elf
  EXPORT EsperantoDeviceMinionRuntimeTargets
  RUNTIME DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/MasterMinion
  COMPONENT master-minion)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.lst
        ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.map
  DESTINATION ${LIB_INSTALL_DIR}/esperanto-fw/MasterMinion
  COMPONENT master-minion)
