# See https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/CrossCompiling

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

set(GCC_PATH /esperanto/minion)

# Only search our cross-tooclhain's library paths
set(CMAKE_FIND_ROOT_PATH ${GCC_PATH})
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

set(CMAKE_AR            ${GCC_PATH}/bin/riscv64-unknown-elf-ar)
set(CMAKE_RANLIB        ${GCC_PATH}/bin/riscv64-unknown-elf-ranlib)
set(CMAKE_C_COMPILER    ${GCC_PATH}/bin/riscv64-unknown-elf-gcc)

