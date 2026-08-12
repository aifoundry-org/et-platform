# etrt-python

Python bindings for the ET Tools Runtime Library (etrt/IRuntime).

> [!WARNING]
> The code is alpha quality, interfaces will almost certainly change.

## Overview

`etrt-python` provides Python bindings for the ET RISC-V accelerator runtime library, enabling:
- Device management and enumeration
- Memory allocation and transfers between host and device
- Kernel loading and execution
- Stream-based asynchronous operations
- Event synchronization and error handling

## Requirements

- Python 3.8+
- CMake 3.15+
- ET platform (runtime and dependencies) installed to `/opt/et` or custom location via `ET_PLATFORM` environment variable
- nanobind (automatically installed via pip)

## Installation

### From source

If ET platform is installed to `/opt/et` (default):

```bash
pip install .
```

If ET platform is in a custom location:

```bash
ET_PLATFORM=/path/to/et pip install .
```

For development (editable install):

```bash
pip install -e .
```

### Manual CMake build

```bash
pip install nanobind
mkdir build && cd build

# If ET platform is at /opt/et (default)
cmake ..

# Or specify custom ET platform location
export ET_PLATFORM=/path/to/et
cmake ..

make
```

## Quick Start

```python
import etrt

# Create device layer (PCIe backend)
device_layer = etrt.create_pcie_device_layer()

# Create runtime
runtime = etrt.Runtime.create(device_layer)

# Get available devices
devices = runtime.get_devices()
device = devices[0]

# Create stream for async operations
stream = runtime.create_stream(device)

# Allocate device memory (returns integer address)
dev_buffer = runtime.malloc_device(device, 1024, 64)

# Load and run kernel
# IMPORTANT: Keep elf_data alive until load completes
with open("kernel.elf", "rb") as f:
    elf_data = f.read()

# load_code returns tuple: (event, kernel_id, load_address)
event, kernel_id, load_address = runtime.load_code(stream, elf_data)

# Wait for code load to complete
runtime.wait_for_event(event, timeout=10.0)

# Launch kernel
# IMPORTANT: Keep kernel_args alive until kernel completes
import struct
kernel_args = struct.pack("II", 0x1234, 0x5678)  # Example arguments
options = etrt.KernelLaunchOptions()
options.set_shire_mask(0xFF)
event = runtime.kernel_launch(stream, kernel_id, kernel_args, options)

# Wait for completion
runtime.wait_for_event(event, timeout=10.0)

# Cleanup
runtime.free_device(device, dev_buffer)
runtime.destroy_stream(stream)
```

## API Overview

### Device Layer Creation

```python
# PCIe hardware backend
device_layer = etrt.create_pcie_device_layer(
    enable_master_minion=True,
    enable_service_processor=False
)

# System emulator backend
sysemu_opts = etrt.SysEmuOptions()
sysemu_opts.master_minion_elf_path = "/path/to/mm.elf"
# ... configure other options
device_layer = etrt.create_sysemu_device_layer(sysemu_opts, num_devices=1)
```

### Runtime Creation

```python
runtime = etrt.Runtime.create(device_layer, options)
```

### Memory Operations

```python
# Allocate device memory (returns integer address)
dev_buffer = runtime.malloc_device(device, size, alignment)

# Transfer host -> device (use any buffer: numpy array, bytes, bytearray, etc.)
import numpy as np
host_data = np.arange(256, dtype=np.uint32)  # 256 * 4 = 1024 bytes
event = runtime.memcpy_host_to_device(stream, host_data, dev_buffer, 1024, barrier=False)
# IMPORTANT: Keep host_data alive until event completes
runtime.wait_for_event(event, timeout=5.0)

# Transfer device -> host (fills provided buffer - numpy array, bytearray, etc.)
output_buffer = np.empty(256, dtype=np.uint32)  # 256 * 4 = 1024 bytes
event = runtime.memcpy_device_to_host(stream, dev_buffer, output_buffer, 1024, barrier=False)
# IMPORTANT: Keep output_buffer alive until event completes
runtime.wait_for_event(event, timeout=5.0)
# Now output_buffer is filled with device data

# Free device memory
runtime.free_device(device, dev_buffer)
```

### Error Handling

```python
# Retrieve errors
errors = runtime.retrieve_stream_errors(stream)
for error in errors:
    print(f"Error code: {error.error_code}")

# Async error callback
def on_error(event_id, error):
    print(f"Async error on event {event_id}: {error.error_code}")

runtime.set_on_stream_errors_callback(on_error)

# Exception handling
try:
    runtime.kernel_launch(stream, kernel_id, args, options)
except etrt.RuntimeError as e:
    print(f"Runtime error: {e}")
```

## Examples

See the `examples/` directory for complete examples:
- `basic_usage.py` - Simple end-to-end workflow
- `sysemu_example.py` - Using system emulator backend
- `async_operations.py` - Asynchronous operations with callbacks

## License

Apache 2.0

## Contributing

Please report issues and submit pull requests on GitHub.
