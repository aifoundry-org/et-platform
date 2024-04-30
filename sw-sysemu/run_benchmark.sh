./make.sh

cmake --preset release -G Ninja

cmake --build --preset release

./build/Release/bench/bench
