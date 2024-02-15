# sw-sysemu micro-benchmark suite

This microbenchmark depends on some device_kernels that must be compiled beforehand.

## How to compile test device_kernels

Firstly ensure you have a riscv-gnu-toolchain compiler in your path.
If you don't have one you can use Conan to download one:
```
conan install riscv-gnu-toolchain/20220720@ -pr:b=default -pr:h=linux-ubuntu22.04-x86_64-gcc11-release -if=toolchain -g VirtualRunEnv
source toolchain/conanrun.sh
```

Then go to `sw-sysemu/bench/device_kernels` folder and compile tests

```
make
```
Device elfs will be present in `build/device_kernels/` dir.

## Compiling sw-sysemu with benchmarks

If compiling with native CMake, make sure to pass `-DBENCHMARKS=ON`.

If using Conan package provide `-o:h sw-sysemu:with_benchmarks=True`

```
conan install . -pr:b=default -pr:h=linux-ubuntu22.04-x86_64-gcc11-release-shared -s:h sw-sysemu:build_type=Release -o sw-sysemu:with_benchmarks=True
cmake --preset release -G Ninja
cmake --build --preset release
```

Then the benchmark will be available in `build/Release/bench/bench`.
Replace Release with other build types (Debug, RelWithDebInfo, etc..) according to the value passed in `-s:h sw-sysemu:build_type`.
