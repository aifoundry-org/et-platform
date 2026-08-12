#!/usr/bin/env python3
"""
Async operations example for etrt-python

Demonstrates:
- Async error callbacks
- Stream error handling
- Event-based synchronization
"""

import etrt


def stream_error_callback(event_id, error):
    """Called when a stream error occurs (may be on background thread)"""
    print(f"\n[CALLBACK] Stream error on event {event_id}:")
    print(f"  Error code: {error.error_code}")
    print(f"  Device: {error.device}")
    if error.stream:
        print(f"  Stream: {error.stream}")
    if error.cm_shire_mask:
        print(f"  Shire mask: 0x{error.cm_shire_mask:X}")


def kernel_aborted_callback(event_id, context_bytes, size, free_resources):
    """Called when a kernel is aborted (may be on background thread)"""
    print(f"\n[CALLBACK] Kernel aborted on event {event_id}:")
    print(f"  Context size: {size} bytes")

    # Parse error context if available
    # (Would need to decode context_bytes as ErrorContext struct)

    # IMPORTANT: Must call free_resources when done
    free_resources()
    print("  Resources freed")


def main():
    # Create device layer and runtime
    print("Creating runtime...")
    device_layer = etrt.create_pcie_device_layer()
    runtime = etrt.Runtime.create(device_layer)

    # Set error callbacks
    print("Setting error callbacks...")
    runtime.set_on_stream_errors_callback(stream_error_callback)
    runtime.set_on_kernel_aborted_error_callback(kernel_aborted_callback)

    # Get device and create stream
    devices = runtime.get_devices()
    if not devices:
        print("No devices available!")
        return

    device = devices[0]
    stream = runtime.create_stream(device)

    # Allocate device memory
    buffer_size = 4096
    dev_buffer = runtime.malloc_device(device, buffer_size, alignment=64)

    # Transfer data host -> device
    import struct
    host_data = struct.pack('I' * 1024, *range(1024))  # Pack 1024 integers

    print(f"\nTransferring {len(host_data)} bytes to device...")
    h2d_event = runtime.memcpy_host_to_device(
        stream, host_data, dev_buffer, len(host_data), barrier=False
    )
    print(f"Transfer initiated, event: {h2d_event}")

    # Wait for transfer to complete
    if runtime.wait_for_event(h2d_event, timeout=5.0):
        print("Transfer completed successfully")
    else:
        print("Transfer timed out!")

    # Check for errors
    errors = runtime.retrieve_stream_errors(stream)
    if errors:
        print(f"\nErrors detected: {len(errors)}")
        for error in errors:
            print(f"  {error}")
    else:
        print("No errors detected")

    # Get DMA info
    dma_info = runtime.get_dma_info(device)
    print(f"\nDMA constraints:")
    print(f"  Max element size: {dma_info.max_element_size} bytes")
    print(f"  Max element count: {dma_info.max_element_count}")

    # Cleanup
    print("\nCleaning up...")
    runtime.free_device(device, dev_buffer)
    runtime.destroy_stream(stream)

    # Clear callbacks
    runtime.set_on_stream_errors_callback(None)
    runtime.set_on_kernel_aborted_error_callback(None)

    print("Done!")


if __name__ == "__main__":
    try:
        main()
    except etrt.RuntimeError as e:
        print(f"Runtime error: {e}")
    except Exception as e:
        print(f"Error: {e}")
        raise
