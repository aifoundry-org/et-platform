# sw-sysemu micro-benchmark suite

This microbenchmark depends on some device_kernels that must be compiled beforehand.

## Helper script

There is a helper script which is created to help compile kernels and execute on SysEMU for benchmarking
  |-> scripts/run_benchmark.sh

## Details of the script

## Compile test device_kernels

Firstly ensure you have a riscv-gnu-toolchain compiler in your path.
If you don't have one you can use Conan to download one:
```
./dock.py --num-devices=0 --image=convoke/ubuntu-22.04-gcc11-conan prompt
conan install riscv-gnu-toolchain/20220720@ -pr:b=default -pr:h=linux-ubuntu22.04-x86_64-gcc11-release -if=toolchain -g VirtualRunEnv
source toolchain/conanrun.sh
```

Then go to `tools/sw-sysemu` folder and compile tests
Here, run a shell script that will compile multiple source/instruction files

```
./make.sh
```
Device elfs will be present in `build/device_kernels/` dir.

## Compile sw-sysemu for benchmarking

If compiling with native CMake, make sure to pass `-DBENCHMARKS=ON`.

WARNING: If sw-sysemu is built with pre-loaded elfs, all microbenchmarks will load these too! Be advised.

If using Conan package provide `-o:h sw-sysemu:with_benchmarks=True`

```
conan install . -pr:b=default -pr:h=linux-ubuntu22.04-x86_64-gcc11-release-shared -s:h sw-sysemu:build_type=Release -o sw-sysemu:with_benchmarks=True
cmake --preset release -G Ninja
cmake --build --preset release
```

## Run kernel with sw-sysemu for benchmarking
The benchmark will be available in `build/Release/bench/bench`.

## Run benchmark with a script
Optionally, you can run `./run_benchmark.sh` from `tools/sw-sysemu` that will compile the src files, benchmarks, and then run the executable.

## Building debug, other versions of executable
Replace Release with other build types (Debug, RelWithDebInfo, etc..) according to the value passed in `-s:h sw-sysemu:build_type`.
