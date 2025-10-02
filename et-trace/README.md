# esperantoTrace

Header-only for et-trace (device trace) layout and related operations.
This library is shared between the device (ET-SOC) and the host machine.

Project layout:

    et-trace/
    ├── include/                    Public header files
    │   └── et-trace/               Directory where public header files live (Users will `include <et-trace/*.h>`)
    │       ├── encoder.h           Encode interface (mainly for the device)
    │       ├── decoder.h           Decode interface (mainly for the host)
    │       └── layout.h            Type definitions (shared)
    └── tests/                      Unit-level tests
        └── common/                 Common test utilities

Note that the device_trace implementation changes depending on whether `MASTER_MINION` is defined or not.
For more information see [include/device_trace_types.h](include/device_trace_types.h).

To get started:

    mkdir build && cd build
    cmake -DET_TRACE_TEST=ON ..
    make
    ctest
