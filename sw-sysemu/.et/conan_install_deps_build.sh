rm -rf build/
conan install . -pr:b=default -pr:h=linux-ubuntu22.04-x86_64-gcc11-debug-shared   --build missing
conan install . -pr:b=default -pr:h=linux-ubuntu22.04-x86_64-gcc11-release-shared --build missing
