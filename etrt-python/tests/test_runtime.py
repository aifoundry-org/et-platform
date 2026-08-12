#!/usr/bin/env python3
"""
Basic tests for etrt Runtime interface
"""

import pytest
import etrt


def test_import():
    """Test that we can import the module"""
    assert hasattr(etrt, 'Runtime')
    assert hasattr(etrt, 'create_pcie_device_layer')


def test_options():
    """Test Options struct"""
    opts = etrt.Options()
    assert hasattr(opts, 'check_memcpy_device_operations')
    assert hasattr(opts, 'check_device_api_version')

    # Test default options
    default_opts = etrt.get_default_options()
    assert default_opts.check_memcpy_device_operations == True
    assert default_opts.check_device_api_version == True


def test_device_error_code_enum():
    """Test DeviceErrorCode enum"""
    assert etrt.DeviceErrorCode.KERNEL_LAUNCH_EXCEPTION
    assert etrt.DeviceErrorCode.DMA_INVALID_ADDRESS
    assert etrt.DeviceErrorCode.UNKNOWN


def test_stream_error():
    """Test StreamError struct"""
    error = etrt.StreamError()
    assert error.error_code == etrt.DeviceErrorCode.UNKNOWN

    error2 = etrt.StreamError(etrt.DeviceErrorCode.DMA_INVALID_ADDRESS)
    assert error2.error_code == etrt.DeviceErrorCode.DMA_INVALID_ADDRESS


def test_kernel_launch_options():
    """Test KernelLaunchOptions"""
    opts = etrt.KernelLaunchOptions()
    opts.set_shire_mask(0xFF)
    opts.set_barrier(True)
    opts.set_flush_l3(False)


# MemcpyList not exposed - has buffer lifetime issues and is not commonly used
# May be added in the future with proper lifetime management


def test_sysemu_options():
    """Test SysEmuOptions struct"""
    opts = etrt.SysEmuOptions()
    opts.master_minion_elf_path = "/path/to/mm.elf"
    opts.max_cycles = 1000000
    opts.minion_shires_mask = 0xFF

    assert opts.master_minion_elf_path == "/path/to/mm.elf"
    assert opts.max_cycles == 1000000
    assert opts.minion_shires_mask == 0xFF


def test_create_pcie_runtime():
    """Test creating runtime with PCIe device layer"""
    device_layer = etrt.create_pcie_device_layer()
    runtime = etrt.Runtime.create(device_layer)
    assert runtime is not None

    devices = runtime.get_devices()
    assert isinstance(devices, list)


def test_memory_transfer():
    """Test host-device-host memory transfers with numpy arrays"""
    import numpy as np

    # Create device layer and runtime
    device_layer = etrt.create_pcie_device_layer()
    runtime = etrt.Runtime.create(device_layer)

    devices = runtime.get_devices()
    assert devices, "No devices available"

    device = devices[0]
    stream = runtime.create_stream(device)

    try:
        # Create test data - 1GB uint32 values
        size = 1024*1024*256
        input_data = np.arange(size, dtype=np.uint32)

        # Allocate device memory (4 bytes per uint32)
        buffer_size = size * 4
        dev_buffer = runtime.malloc_device(device, buffer_size, alignment=64)

        # Transfer host -> device (pass numpy array directly, not tobytes())
        h2d_event = runtime.memcpy_host_to_device(
            stream, input_data, dev_buffer, buffer_size, barrier=False
        )
        assert runtime.wait_for_event(h2d_event, timeout=5.0), "Host to device transfer timed out"

        # Transfer device -> host (fills provided buffer)
        output_data = np.empty(size, dtype=np.uint32)
        d2h_event = runtime.memcpy_device_to_host(
            stream, dev_buffer, output_data, buffer_size, barrier=False
        )
        assert runtime.wait_for_event(d2h_event, timeout=5.0), "Device to host transfer timed out"

        # Verify data matches
        assert output_data.shape == input_data.shape, "Shape mismatch"
        assert np.array_equal(input_data, output_data), "Data mismatch after round-trip transfer"

        # Cleanup
        runtime.free_device(device, dev_buffer)

    finally:
        runtime.destroy_stream(stream)


def test_kernel_execution():
    """Test loading and executing a kernel (memops memset)"""
    import numpy as np
    import struct
    import os

    # Create device layer and runtime
    device_layer = etrt.create_pcie_device_layer()
    runtime = etrt.Runtime.create(device_layer)

    devices = runtime.get_devices()
    assert devices, "No devices available"

    device = devices[0]
    stream = runtime.create_stream(device)

    try:
        # Load the memops kernel
        # IMPORTANT: Keep elf_data alive until load completes
        kernel_path = os.path.join(os.path.dirname(__file__), "kernels", "memops.elf")
        with open(kernel_path, "rb") as f:
            elf_data = f.read()

        load_event, kernel_id, load_address = runtime.load_code(stream, elf_data)
        assert runtime.wait_for_event(load_event, timeout=10.0), "Kernel load timed out"
        # elf_data can be released now

        # Allocate device buffer (1KB)
        buffer_size = 1024
        dev_buffer = runtime.malloc_device(device, buffer_size, alignment=64)

        # Prepare kernel parameters matching memset_params struct
        # struct memset_params {
        #     uint32_t op_type;      // GGML_ET_MEMOP_MEMSET = 0
        #     uint32_t value;        // Value to set
        #     void* dst_ptr;         // Destination device pointer (8 bytes)
        #     size_t size;           // Number of bytes to set (8 bytes)
        # };
        # IMPORTANT: Keep params alive until kernel completes
        memset_value = 0x42  # Random known value
        params = struct.pack(
            "II",      # op_type (uint32), value (uint32)
            0,         # op_type = GGML_ET_MEMOP_MEMSET = 0
            memset_value
        )
        params += struct.pack("Q", dev_buffer)  # dst_ptr (void*, 8 bytes)
        params += struct.pack("Q", buffer_size)  # size (size_t, 8 bytes)

        # Launch kernel
        options = etrt.KernelLaunchOptions()
        options.set_shire_mask(0xFF)
        options.set_barrier(True)
        options.set_flush_l3(False)

        kernel_event = runtime.kernel_launch(stream, kernel_id, params, options)
        assert runtime.wait_for_event(kernel_event, timeout=10.0), "Kernel execution timed out"
        # params can be released now

        # Read back the buffer and verify it was memset correctly
        output_data = np.empty(buffer_size, dtype=np.uint8)
        d2h_event = runtime.memcpy_device_to_host(
            stream, dev_buffer, output_data, buffer_size, barrier=False
        )
        assert runtime.wait_for_event(d2h_event, timeout=5.0), "Device to host transfer timed out"

        # Verify all bytes are set to the expected value
        assert np.all(output_data == memset_value), f"Memset verification failed: expected all 0x{memset_value:02x}, got varying values"

        # Cleanup
        runtime.unload_code(kernel_id)
        runtime.free_device(device, dev_buffer)

    finally:
        runtime.destroy_stream(stream)


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
