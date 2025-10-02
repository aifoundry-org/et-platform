rm -rf build/
conan install . -pr:b=default -pr:h=linux-ubuntu22.04-x86_64-gcc11-debug-shared   --build missing -o et-host-utils:fvisibility=default -o et-host-utils:with_tests=True
conan install . -pr:b=default -pr:h=linux-ubuntu22.04-x86_64-gcc11-release-shared --build missing -o et-host-utils:fvisibility=hidden  -o et-host-utils:with_tests=True
