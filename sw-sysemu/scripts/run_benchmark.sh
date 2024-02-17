# Make sure to be in docker
#./dock.py --num-devices=0 --image=convoke/ubuntu-22.04-gcc11-conan prompt

# Install the RISCV toolchain
conan install riscv-gnu-toolchain/20220720@ -pr:b=default -pr:h=linux-ubuntu22.04-x86_64-gcc11-release -if=toolchain -g VirtualRunEnv
source toolchain/conanrun.sh

# Compile unit tests for benchmarking
cd bench/device_kernels
make

# Compile SysEMu for benchmarking
cd ../../.
conan install . -pr:b=default -pr:h=linux-ubuntu22.04-x86_64-gcc11-release-shared -s:h sw-sysemu:build_type=Release -o sw-sysemu:with_benchmarks=True
cmake --preset release -G Ninja
cmake --build --preset release

# Run benchmark
./build/Release/bench/bench

