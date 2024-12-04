rm -rf build/
conan install . -pr:b=default -pr:h=linux-ubuntu22.04-x86_64-gcc11-debug-shared   -if build/Debug          --build missing -o et-host-utils:fvisibility=hidden -o runtime:fvisibility=hidden -o runtime:with_tools=True -o runtime:with_tests=True -s:h sw-sysemu:build_type=RelWithDebInfo
conan install . -pr:b=default -pr:h=linux-ubuntu22.04-x86_64-gcc11-release-shared -if build/Release        --build missing -o et-host-utils:fvisibility=hidden -o runtime:fvisibility=hidden
conan install . -pr:b=default -pr:h=linux-ubuntu22.04-x86_64-gcc11-release-shared -if build/RelWithDebInfo --build missing -o et-host-utils:fvisibility=hidden -o runtime:fvisibility=hidden
