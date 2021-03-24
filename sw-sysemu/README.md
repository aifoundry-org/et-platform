SW SYSEMU
========

Functional ETSOC-1 emulator.
Able to execute RISCV instructions with Esperanto Technologies.

### Installation

Make sure you track the default development branch:

    git checkout develop/sw-sysemu

Compile using the standard CMake flow:

    mkdir build
    cd build
    cmake ..
    make


The `sys_emu` binary will be built in the root of the `build` directory.

You may need installing some packages in order to fullfil sysemu dependencies.
For instance, sysmeu assumes libunwind will be present in the library path.
Make sure that that library is available in the Docker image or in the compilation environment.

### Basic usage

We can load any binary (elf) in sys_emu using the `-elf_load` option.

    ./sys_emu -elf_load <path>

Note that sys_emu offers a wide set of options which description is available with help:

    ./sys_emu --help

One thing to consider is that sys_emu assumes that we are running a baremetal machine.
This means that, in order to execute a compute kernel, we may need to program the machine to be in a stable state.
This can be achieved by running a set of assembly instructions that clean up registers and prepare the machine for computation.
These set of instructions can be executed by the firmware or by the user, which should add them in the entrypoint before run the compute kernel.






