cmake -S . -B build/Release -G Ninja -DCMAKE_TOOLCHAIN_FILE=$PWD/build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
source build/Release/generators/conanrun.sh
cmake --build build/Release
source build/Release/generators/deactivate_conanrun.sh

cmake -S . -B build/Debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=$PWD/build/Debug/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
source build/Debug/generators/conanrun.sh
cmake --build build/Debug
source build/Debug/generators/deactivate_conanrun.sh
