/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#include <nanobind/nanobind.h>
#include <runtime/Types.h>
#include <cstdio>

namespace nb = nanobind;

// Thread-safe wrapper for StreamErrorCallback
// Callbacks may be invoked from background threads, so we need to acquire the GIL
rt::StreamErrorCallback make_thread_safe_stream_error_callback(nb::object py_callback) {
  // Keep the Python callable alive
  nb::object callback_copy = py_callback;

  return [callback_copy](rt::EventId event, const rt::StreamError& error) {
    // Acquire GIL before calling Python code
    nb::gil_scoped_acquire gil;

    try {
      callback_copy(event, error);
    } catch (const nb::python_error& e) {
      // Python exception occurred in callback
      // We can't propagate it (we're in a C++ callback context)
      // So we print it and continue
      PyErr_Print();
    } catch (const std::exception& e) {
      // C++ exception in callback - print to stderr
      fprintf(stderr, "Exception in stream error callback: %s\n", e.what());
    }
  };
}

// Thread-safe wrapper for KernelAbortedCallback
rt::KernelAbortedCallback make_thread_safe_kernel_aborted_callback(nb::object py_callback) {
  // Keep the Python callable alive
  nb::object callback_copy = py_callback;

  return [callback_copy](rt::EventId event, std::byte* context, size_t size,
                        std::function<void()> freeResources) {
    // Acquire GIL before calling Python code
    nb::gil_scoped_acquire gil;

    try {
      // Convert context to Python bytes
      nb::bytes context_bytes(reinterpret_cast<const char*>(context), size);

      // Create Python wrapper for freeResources
      auto free_fn = [freeResources]() {
        // Release GIL when calling C++ code
        nb::gil_scoped_release gil_release;
        freeResources();
      };

      // Call Python callback
      callback_copy(event, context_bytes, size, nb::cpp_function(free_fn));

    } catch (const nb::python_error& e) {
      // Python exception occurred in callback
      PyErr_Print();
      // Still need to free resources
      freeResources();
    } catch (const std::exception& e) {
      // C++ exception in callback - print to stderr
      fprintf(stderr, "Exception in kernel aborted callback: %s\n", e.what());
      // Still need to free resources
      freeResources();
    }
  };
}
