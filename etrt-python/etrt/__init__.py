"""
ET Tools Runtime Library (etrt) - Python Bindings

Python bindings for the ET RISC-V accelerator runtime library.

Basic usage:
    import etrt

    # Create device layer
    device_layer = etrt.create_pcie_device_layer()

    # Create runtime
    runtime = etrt.Runtime.create(device_layer)

    # Get devices and create stream
    devices = runtime.get_devices()
    stream = runtime.create_stream(devices[0])

    # Allocate memory, load code, execute kernels...
"""

# Import native module
from ._etrt_native import *

__version__ = "0.1.0"
__all__ = [
    # Exception
    "RuntimeError",

    # Handle types
    "EventId",
    "StreamId",
    "DeviceId",
    "KernelId",
    "DeviceBuffer",

    # Configuration
    "Options",
    "get_default_options",

    # Enums
    "DeviceErrorCode",
    "FormFactor",
    "ArchRevision",

    # Structs
    "ErrorContext",
    "StreamError",
    "LoadCodeResult",
    "DeviceProperties",
    "UserTrace",
    "DmaInfo",
    "MemcpyList",
    "KernelLaunchOptions",

    # Device layer
    "SysEmuOptions",
    "IDeviceLayer",
    "create_pcie_device_layer",
    "create_sysemu_device_layer",

    # Runtime
    "Runtime",
]
