#------------------------------------------------------------------------------
# Copyright (c) 2026 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0
#------------------------------------------------------------------------------
#
# erbium_launcher - generic host launcher for Erbium kernels.
# Loads any Erbium-flavored kernel ELF and runs it on a single shire of
# ET-SoC-1 silicon (or sys_emu).  Pairs with the contrib/erbium_run
# wrapper that adds erbium_emu support and offset->address translation.
#

add_executable(erbium_launcher main.cpp)

target_link_libraries(erbium_launcher
    PRIVATE
        deviceLayer::deviceLayer
        runtime::etrt
        hostUtils::logging
        sw-sysemu::sw-sysemu
)

target_compile_definitions(erbium_launcher
    PRIVATE
        ET_PLATFORM_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
)

install(TARGETS erbium_launcher
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Ship the erbium_run python wrapper alongside the launcher binary it drives.
install(PROGRAMS ${CMAKE_SOURCE_DIR}/../contrib/erbium_run
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
