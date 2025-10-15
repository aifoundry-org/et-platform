# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0

set(TRACE_ENABLED TRUE)

test_kernel(
  NAME trace
  SOURCES trace.c
  INCLUDES include
  )
