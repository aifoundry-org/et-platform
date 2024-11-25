cmake --preset release -G Ninja -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
source build/Release/generators/conanbuild.sh
cmake --build --preset release
source build/Release/generators/deactivate_conanbuild.sh

cmake --preset debug   -G Ninja -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
source build/Debug/generators/conanbuild.sh
cmake --build --preset debug
source build/Debug/generators/deactivate_conanbuild.sh
