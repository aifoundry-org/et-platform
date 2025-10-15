# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0


set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/sections.ld)
test_kernel(
  NAME bss
  SOURCES bss.c
  )
unset(LINKER_SCRIPT)