/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#include <nanobind/nanobind.h>

namespace nb = nanobind;

// Forward declarations of binding functions
void bind_types(nb::module_& m);
void bind_device_layer(nb::module_& m);
void bind_runtime(nb::module_& m);

NB_MODULE(_etrt_native, m) {
  m.doc() = "Python bindings for ET Tools Runtime Library (etrt)";

  bind_types(m);
  bind_device_layer(m);
  bind_runtime(m);
}
