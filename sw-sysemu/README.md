SW SYSEMU
========

SW sysemu is the functional ETSOC-1 emulator.
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

We can load any binary (elf) in sys_emu using the `-elf` option.

    ./sys_emu -elf <path>

Note that sys_emu offers a wide set of options which description is available with help:

    ./sys_emu --help

One thing to consider is that sys_emu assumes that we are running a baremetal machine.
This means that, in order to execute a compute kernel, we may need to program the machine to be in a stable state.
This can be achieved by running a set of assembly instructions that clean up registers and prepare the machine for computation.
These set of instructions can be executed by the firmware or by the user, which should add them in the entrypoint before run the compute kernel.

### Compiling kernels

As mentioned above, compute kernels must introduce a set of sequences to clean up the state of the baremetal execution.
To this end, we may want to introduce a sequence of instructions in the kernel entrypoint to fill that purpose. 
We can follow the approach of the `test.c` kernel placed in `examples/src` directory.
That test has a main that executes a simple RISCV instruction.
However, in order to execute this kernel in sysemu, we need to link the kernel against `boot.S` and `crt.S`.
For more info, see `examples/Makefiles`.
Note that we can generate a binary of `test.c` just doing:

     cd examples && make

This will generate a `test.elf` binary in `build/examples`.

### Running kernels
 
We can easily execute binaries using our functional emulator. We just need to call it using the right hyperparameters.
Besides the `-elf` option, we may want to enforce which chip configuration we are going to use or which is the initial PC.
In the following example, we define our entrypoint in address 0x8000001000, and we run single-threaded single-minion configuration.
Also, we enable logging by using the `-l` option. 

    ./sys_emu -l -reset_pc 0x8000001000 -single_thread -minions 0x1 -shires 0x1 -elf examples/test.elf

Note that, in the execution log, we can see the result of the executed compute kernel (in that case, a single instruction).

    99: DEBUG EMU: [H0 S0:N0:C0:T0] Fetched instruction from PC 0x80008000f8: 0x5ad00f93
    99: DEBUG EMU: [H0 S0:N0:C0:T0] I(M): addi x31,x0,1453
    99: DEBUG EMU: [H0 S0:N0:C0:T0] 	x31 = 0x5ad

To ease the development flow, a riscv-gcc compiled version of test.c has been added in its binary form in `examples/bin/test.elf`.
