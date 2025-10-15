# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0

set(TRACE_ENABLED TRUE)

test_kernel(
  NAME trace_ring_buffer
  SOURCES trace_ring_buffer.c
  INCLUDES include
  )
