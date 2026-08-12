#!/usr/bin/env python3
"""
System emulator example for etrt-python

Demonstrates using the sysemu device layer backend
"""

import etrt


def main():
    # Configure sysemu options
    print("Configuring system emulator...")
    sysemu_opts = etrt.SysEmuOptions()

    # Set paths to firmware binaries (adjust these paths as needed)
    sysemu_opts.bootrom_trampoline_to_bl2_elf_path = "/path/to/bootrom_trampoline.elf"
    sysemu_opts.sp_bl2_elf_path = "/path/to/sp_bl2.elf"
    sysemu_opts.machine_minion_elf_path = "/path/to/machine_minion.elf"
    sysemu_opts.master_minion_elf_path = "/path/to/master_minion.elf"
    sysemu_opts.worker_minion_elf_path = "/path/to/worker_minion.elf"

    # Set sysemu executable and working directory
    sysemu_opts.executable_path = "/path/to/sysemu"
    sysemu_opts.run_dir = "/tmp/sysemu_run"

    # Configure simulation parameters
    sysemu_opts.max_cycles = 10000000  # 10M cycles max
    sysemu_opts.minion_shires_mask = 0xFF  # Enable 8 shires

    # Set UART log path
    sysemu_opts.pu_uart0_path = "/tmp/uart0.log"

    # Create sysemu device layer
    print("Creating sysemu device layer...")
    device_layer = etrt.create_sysemu_device_layer(sysemu_opts, num_devices=1)

    # Create runtime
    print("Creating runtime...")
    runtime = etrt.Runtime.create(device_layer)

    # Get devices
    devices = runtime.get_devices()
    print(f"Found {len(devices)} device(s)")

    if not devices:
        print("No devices available!")
        return

    device = devices[0]

    # Get device properties
    props = runtime.get_device_properties(device)
    print(f"Device properties:")
    print(f"  Frequency: {props.frequency} MHz")
    print(f"  Available shires: {props.available_shires}")
    print(f"  Memory size: {props.memory_size} bytes")

    # Create stream and do work...
    stream = runtime.create_stream(device)

    # Cleanup
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
