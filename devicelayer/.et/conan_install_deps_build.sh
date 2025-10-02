rm -rf build/
conan install . -pr:b=default -pr:h=linux-ubuntu22.04-x86_64-gcc11-debug-shared   -o deviceLayer:fvisibility=default --build missing -o deviceLayer:with_tests=True
conan install . -pr:b=default -pr:h=linux-ubuntu22.04-x86_64-gcc11-release-shared -o deviceLayer:fvisibility=hidden  --build missing
