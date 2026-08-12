/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#include <nanobind/nanobind.h>
#include <nanobind/stl/chrono.h>
#include <nanobind/stl/function.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/vector.h>

#include <device-layer/IDeviceLayer.h>
#include <runtime/IRuntime.h>
#include <runtime/Types.h>

#include <chrono>

namespace nb = nanobind;

// Forward declaration
rt::StreamErrorCallback make_thread_safe_stream_error_callback(nb::object py_callback);
rt::KernelAbortedCallback make_thread_safe_kernel_aborted_callback(nb::object py_callback);

void bind_runtime(nb::module_& m) {
  // Vector types are automatically handled by STL type casters

  // IRuntime class (unique_ptr is handled automatically by nanobind)
  nb::class_<rt::IRuntime>(m, "Runtime", "ET Tools Runtime interface")

    // Factory method (static)
    .def_static("create",
                [](std::shared_ptr<dev::IDeviceLayer> device_layer,
                   const rt::Options& options) -> std::unique_ptr<rt::IRuntime> {
                  return rt::IRuntime::create(device_layer, options);
                },
                nb::arg("device_layer"),
                nb::arg("options") = rt::getDefaultOptions(),
                "Create runtime with device layer")

    // Device management
    .def("get_devices", [](rt::IRuntime& self) -> std::vector<int> {
           auto devices = self.getDevices();
           std::vector<int> result;
           result.reserve(devices.size());
           for (auto device : devices) {
             result.push_back(static_cast<int>(device));
           }
           return result;
         },
         "Get list of available devices")

    .def("get_device_properties", [](rt::IRuntime& self, int device) {
           return self.getDeviceProperties(rt::DeviceId{device});
         },
         nb::arg("device"),
         "Get device hardware properties")

    .def("is_p2p_enabled", [](rt::IRuntime& self, int device1, int device2) {
           return self.isP2PEnabled(rt::DeviceId{device1}, rt::DeviceId{device2});
         },
         nb::arg("device1"),
         nb::arg("device2"),
         "Check if peer-to-peer DMA is enabled between devices")

    // Memory management
    .def("malloc_device",
         [](rt::IRuntime& self, int device, size_t size, uint32_t alignment) -> uintptr_t {
           auto ptr = self.mallocDevice(rt::DeviceId{device}, size, alignment);
           return reinterpret_cast<uintptr_t>(ptr);
         },
         nb::arg("device"),
         nb::arg("size"),
         nb::arg("alignment") = 64,
         "Allocate device memory - returns opaque pointer as integer")

    .def("free_device",
         [](rt::IRuntime& self, int device, uintptr_t buffer) {
           self.freeDevice(rt::DeviceId{device}, reinterpret_cast<std::byte*>(buffer));
         },
         nb::arg("device"),
         nb::arg("buffer"),
         "Free device memory")

    // Stream management
    .def("create_stream", [](rt::IRuntime& self, int device) -> int {
           return static_cast<int>(self.createStream(rt::DeviceId{device}));
         },
         nb::arg("device"),
         "Create execution stream")

    .def("destroy_stream", [](rt::IRuntime& self, int stream) {
           self.destroyStream(rt::StreamId{stream});
         },
         nb::arg("stream"),
         "Destroy execution stream")

    // Code loading
    .def("load_code",
         [](rt::IRuntime& self, int stream, nb::object elf_obj) {
           // Get readable pointer from buffer (bytes, bytearray, etc.)
           PyObject* elf_ptr = elf_obj.ptr();
           Py_buffer view;
           if (PyObject_GetBuffer(elf_ptr, &view, PyBUF_SIMPLE) != 0) {
             PyErr_Clear();
             throw std::runtime_error("ELF data must be a buffer (bytes, bytearray, etc.)");
           }

           // Queue the code load (async DMA)
           // Keep the buffer view alive during the call
           try {
             auto result = self.loadCode(rt::StreamId{stream},
                                          reinterpret_cast<const std::byte*>(view.buf),
                                          view.len);
             PyBuffer_Release(&view);
             // Convert LoadCodeResult: enum IDs to ints, pointer to uintptr_t
             return std::make_tuple(
               static_cast<int>(result.event_),
               static_cast<int>(result.kernel_),
               reinterpret_cast<uintptr_t>(result.loadAddress_)
             );
           } catch (const std::exception& e) {
             PyBuffer_Release(&view);
             throw std::runtime_error(std::string("loadCode failed: ") + e.what());
           }
         },
         nb::arg("stream"),
         nb::arg("elf_data"),
         "Load ELF kernel code - returns (event, kernel, load_address) tuple")

    .def("unload_code", [](rt::IRuntime& self, int kernel) {
           self.unloadCode(rt::KernelId{kernel});
         },
         nb::arg("kernel"),
         "Unload kernel code")

    // Kernel execution
    .def("kernel_launch",
         [](rt::IRuntime& self, int stream, int kernel,
            nb::object args_obj, const rt::KernelLaunchOptions& options) -> int {
           // Get readable pointer from buffer (bytes, bytearray, etc.)
           PyObject* args_ptr = args_obj.ptr();
           Py_buffer view;
           if (PyObject_GetBuffer(args_ptr, &view, PyBUF_SIMPLE) != 0) {
             PyErr_Clear();
             throw std::runtime_error("Kernel args must be a buffer (bytes, bytearray, etc.)");
           }

           void* buf_ptr = view.buf;
           size_t args_size = view.len;
           PyBuffer_Release(&view);

           // Queue the kernel launch (async)
           // NOTE: The buffer pointer remains valid as long as the Python object (args_obj) is alive.
           auto event = self.kernelLaunch(rt::StreamId{stream}, rt::KernelId{kernel},
                                   reinterpret_cast<const std::byte*>(buf_ptr),
                                   args_size, options);
           return static_cast<int>(event);
         },
         nb::arg("stream"),
         nb::arg("kernel"),
         nb::arg("args"),
         nb::arg("options"),
         "Launch kernel with options")

    // Memory transfers - host to device
    .def("memcpy_host_to_device",
         [](rt::IRuntime& self, int stream, nb::object src_obj,
            uintptr_t dst, size_t size, bool barrier) -> int {
           // Get readable pointer from buffer (numpy array, bytes, bytearray, etc.)
           PyObject* src_ptr = src_obj.ptr();
           Py_buffer view;
           if (PyObject_GetBuffer(src_ptr, &view, PyBUF_SIMPLE) != 0) {
             PyErr_Clear();
             throw std::runtime_error("Source must be a buffer (numpy array, bytes, bytearray, etc.)");
           }

           // Check buffer size
           if (static_cast<size_t>(view.len) < size) {
             PyBuffer_Release(&view);
             throw std::runtime_error("Source buffer too small");
           }

           void* buf_ptr = view.buf;
           PyBuffer_Release(&view);

           // Queue the DMA transfer
           // NOTE: The buffer pointer remains valid as long as the Python object (src_obj) is alive.
           // The caller must ensure src_obj is not garbage collected before waiting for the event.
           auto event = self.memcpyHostToDevice(rt::StreamId{stream},
                                         reinterpret_cast<const std::byte*>(buf_ptr),
                                         reinterpret_cast<std::byte*>(dst), size, barrier, rt::defaultCmaCopyFunction);
           return static_cast<int>(event);
         },
         nb::arg("stream"),
         nb::arg("src"),
         nb::arg("dst"),
         nb::arg("size"),
         nb::arg("barrier") = false,
         "Copy data from host to device (single operation)")

    // Memory transfers - device to host
    .def("memcpy_device_to_host",
         [](rt::IRuntime& self, int stream, uintptr_t src, nb::object dst_obj,
            size_t size, bool barrier) -> int {
           // Get writable pointer from buffer (numpy array, bytearray, etc.)
           PyObject* dst_ptr = dst_obj.ptr();
           Py_buffer view;
           if (PyObject_GetBuffer(dst_ptr, &view, PyBUF_WRITABLE) != 0) {
             PyErr_Clear();
             throw std::runtime_error("Destination must be a writable buffer (numpy array, bytearray, etc.)");
           }

           // Check buffer size
           if (static_cast<size_t>(view.len) < size) {
             PyBuffer_Release(&view);
             throw std::runtime_error("Destination buffer too small");
           }

           void* buf_ptr = view.buf;
           PyBuffer_Release(&view);

           // Queue the DMA transfer
           // NOTE: The buffer pointer remains valid as long as the Python object (dst_obj) is alive.
           // The caller must ensure dst_obj is not garbage collected before waiting for the event.
           auto event = self.memcpyDeviceToHost(rt::StreamId{stream},
                                                 reinterpret_cast<std::byte*>(src),
                                                 reinterpret_cast<std::byte*>(buf_ptr),
                                                 size, barrier, rt::defaultCmaCopyFunction);
           return static_cast<int>(event);
         },
         nb::arg("stream"),
         nb::arg("src"),
         nb::arg("dst"),
         nb::arg("size"),
         nb::arg("barrier") = false,
         "Copy data from device to host into provided buffer")

    // Memory transfers - device to device (source stream)
    .def("memcpy_device_to_device",
         [](rt::IRuntime& self, int stream_src, int device_dst,
            uintptr_t src, uintptr_t dst, size_t size, bool barrier) -> int {
           auto event = self.memcpyDeviceToDevice(rt::StreamId{stream_src}, rt::DeviceId{device_dst},
                                                   reinterpret_cast<std::byte*>(src),
                                                   reinterpret_cast<std::byte*>(dst),
                                                   size, barrier);
           return static_cast<int>(event);
         },
         nb::arg("stream_src"),
         nb::arg("device_dst"),
         nb::arg("src"),
         nb::arg("dst"),
         nb::arg("size"),
         nb::arg("barrier") = false,
         "Copy data from device to device (source stream)")

    // Memory transfers - device to device (destination stream)
    .def("memcpy_device_to_device",
         [](rt::IRuntime& self, int device_src, int stream_dst,
            uintptr_t src, uintptr_t dst, size_t size, bool barrier) -> int {
           auto event = self.memcpyDeviceToDevice(rt::DeviceId{device_src}, rt::StreamId{stream_dst},
                                                   reinterpret_cast<std::byte*>(src),
                                                   reinterpret_cast<std::byte*>(dst),
                                                   size, barrier);
           return static_cast<int>(event);
         },
         nb::arg("device_src"),
         nb::arg("stream_dst"),
         nb::arg("src"),
         nb::arg("dst"),
         nb::arg("size"),
         nb::arg("barrier") = false,
         "Copy data from device to device (destination stream)")

    // Synchronization
    .def("wait_for_event",
         [](rt::IRuntime& self, int event, double timeout_seconds) -> bool {
           auto timeout = std::chrono::duration<double>(timeout_seconds);
           auto timeout_s = std::chrono::duration_cast<std::chrono::seconds>(timeout);
           return self.waitForEvent(rt::EventId{static_cast<uint16_t>(event)}, timeout_s);
         },
         nb::arg("event"),
         nb::arg("timeout") = 60.0,
         "Wait for event completion")

    .def("wait_for_stream",
         [](rt::IRuntime& self, int stream, double timeout_seconds) -> bool {
           auto timeout = std::chrono::duration<double>(timeout_seconds);
           auto timeout_s = std::chrono::duration_cast<std::chrono::seconds>(timeout);
           return self.waitForStream(rt::StreamId{stream}, timeout_s);
         },
         nb::arg("stream"),
         nb::arg("timeout") = 60.0,
         "Wait for all stream operations to complete")

    // Error handling
    .def("retrieve_stream_errors", [](rt::IRuntime& self, int stream) {
           return self.retrieveStreamErrors(rt::StreamId{stream});
         },
         nb::arg("stream"),
         "Retrieve errors from stream")

    .def("set_on_stream_errors_callback",
         [](rt::IRuntime& self, const nb::object& callback) {
           if (callback.is_none()) {
             self.setOnStreamErrorsCallback(rt::StreamErrorCallback{});
           } else {
             self.setOnStreamErrorsCallback(make_thread_safe_stream_error_callback(callback));
           }
         },
         nb::arg("callback") = nb::none(),
         "Set async error callback (or None to clear)")

    .def("set_on_kernel_aborted_error_callback",
         [](rt::IRuntime& self, const nb::object& callback) {
           if (callback.is_none()) {
             self.setOnKernelAbortedErrorCallback(rt::KernelAbortedCallback{});
           } else {
             self.setOnKernelAbortedErrorCallback(make_thread_safe_kernel_aborted_callback(callback));
           }
         },
         nb::arg("callback") = nb::none(),
         "Set kernel aborted callback (or None to clear)")

    // Command abortion
    .def("abort_command",
         [](rt::IRuntime& self, int command_id, double timeout_ms) -> int {
           auto timeout = std::chrono::duration<double, std::milli>(timeout_ms);
           auto timeout_msec = std::chrono::duration_cast<std::chrono::milliseconds>(timeout);
           auto event = self.abortCommand(rt::EventId{static_cast<uint16_t>(command_id)}, timeout_msec);
           return static_cast<int>(event);
         },
         nb::arg("command_id"),
         nb::arg("timeout_ms") = 1000.0,
         "Abort specific command")

    .def("abort_stream", [](rt::IRuntime& self, int stream) {
           self.abortStream(rt::StreamId{stream});
         },
         nb::arg("stream"),
         "Abort all commands in stream")

    // DMA information
    .def("get_dma_info", [](rt::IRuntime& self, int device) {
           return self.getDmaInfo(rt::DeviceId{device});
         },
         nb::arg("device"),
         "Get DMA constraints for device");
}
