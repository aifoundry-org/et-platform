# Copyright (c) 2026 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0

set(HIST_ITERATIONS 2 CACHE STRING "Number of histogram iterations (for benchmarking)")

# Same source, two backends.  ELFs are install-suffixed by backend so
# they don't collide in /opt/et/kernels/.  test_histogram.py picks the
# right one per --device.
add_erbium_test_kernel(histogram.erbium-soc1sim.elf
  SOURCES histogram.c BACKEND erbium-soc1sim)
add_erbium_test_kernel(histogram.erbium.elf
  SOURCES histogram.c BACKEND erbium)

target_compile_definitions(histogram.erbium-soc1sim.elf PRIVATE HIST_ITERATIONS=${HIST_ITERATIONS})
target_compile_definitions(histogram.erbium.elf         PRIVATE HIST_ITERATIONS=${HIST_ITERATIONS})
