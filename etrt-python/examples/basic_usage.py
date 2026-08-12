#!/usr/bin/env python3
"""
Basic usage example for etrt-python

Demonstrates:
- Creating a runtime with PCIe device layer
- Enumerating devices
- Allocating device memory
- Creating streams
- Loading and executing kernels
- Synchronization
"""

import etrt


def main():
    # Create PCIe device layer
    print("Creating PCIe device layer...")
    device_layer = etrt.create_pcie_device_layer(
        enable_master_minion=True,
        enable_service_processor=False
    )

    # Create runtime with default options
    print("Creating runtime...")
    options = etrt.get_default_options()
    runtime = etrt.Runtime.create(device_layer, options)

    # Get available devices
    devices = runtime.get_devices()
    print(f"Found {len(devices)} device(s)")

    if not devices:
        print("No devices available!")
        return

    device = devices[0]
    print(f"Using device: {device}")

    # Get device properties
    props = runtime.get_device_properties(device)
    print(f"Device properties:")
    print(f"  Frequency: {props.frequency} MHz")
    print(f"  Available shires: {props.available_shires}")
    print(f"  Memory size: {props.memory_size} bytes")
    print(f"  Architecture: {props.device_arch}")

    # Create stream
    print("\nCreating stream...")
    stream = runtime.create_stream(device)

    # Allocate device memory
    buffer_size = 1024 * 1024  # 1 MB
    print(f"Allocating {buffer_size} bytes of device memory...")
    dev_buffer = runtime.malloc_device(device, buffer_size, alignment=64)
    print(f"Allocated device buffer: {dev_buffer}")

    # Load kernel (example - you'll need an actual ELF file)
    # with open("kernel.elf", "rb") as f:
    #     elf_data = f.read()
    #
    # print("\nLoading kernel...")
    # event, kernel_id, load_address = runtime.load_code(stream, elf_data)
    # print(f"Kernel loaded: {kernel_id}")
    # print(f"Load address: 0x{load_address:X}")
    #
    # # Wait for load to complete
    # if runtime.wait_for_event(event, timeout=10.0):
    #     print("Kernel load completed")
    # else:
    #     print("Kernel load timed out!")
    #
    # # Configure kernel launch options
    # options = etrt.KernelLaunchOptions()
    # options.set_shire_mask(0xFF)  # Use all available shires
    # options.set_barrier(True)     # Wait for previous operations
    # options.set_flush_l3(False)   # Don't flush L3
    #
    # # Launch kernel
    # print("\nLaunching kernel...")
    # kernel_args = b""  # Empty args for this example
    # event = runtime.kernel_launch(stream, kernel_id, kernel_args, options)
    # print(f"Kernel launched, event: {event}")
    #
    # # Wait for kernel completion
    # if runtime.wait_for_event(event, timeout=30.0):
    #     print("Kernel execution completed")
    # else:
    #     print("Kernel execution timed out!")
    #
    # # Check for errors
    # errors = runtime.retrieve_stream_errors(stream)
    # if errors:
    #     print(f"\nStream errors detected: {len(errors)}")
    #     for error in errors:
    #         print(f"  {error}")
    # else:
    #     print("\nNo errors detected")
    #
    # # Unload kernel
    # runtime.unload_code(kernel_id)

    # Cleanup
    print("\nCleaning up...")
    runtime.free_device(device, dev_buffer)
    runtime.destroy_stream(stream)
    print("Done!")


if __name__ == "__main__":
    try:
        main()
    except etrt.RuntimeError as e:
        print(f"Runtime error: {e}")
    except Exception as e:
        print(f"Error: {e}")
        raise
