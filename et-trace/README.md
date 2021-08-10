# Device Trace

Header-only for device_trace layout and related operations.
This library is shared between the device (ET-SOC) and the host machine.

Project layout:

    device_trace/
    ├── include/                    Public header files
    │   ├── device_trace.h          Encode interface (mainly for the device)
    │   ├── device_trace_decode.h   Decode interface (mainly for the host)
    │   └── device_trace_types.h    Type definitions (shared)
    └── tests/                      Unit-level tests
        └── common/                 Common test utilities

Note that the device_trace implementation changes depending on whether `MASTER_MINION` is defined or not.
For more information see [include/device_trace_types.h](include/device_trace_types.h).

To get started:

    mkdir build && cd build
    cmake -DDTRACE_TEST=ON ..
    make
    ctest
