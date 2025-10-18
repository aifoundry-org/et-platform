# Findgflags.cmake
find_package(PkgConfig REQUIRED)

pkg_check_modules(gflags REQUIRED gflags)

if(NOT TARGET gflags::gflags)
    add_library(gflags::gflags INTERFACE IMPORTED)
    set_target_properties(gflags::gflags PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${gflags_INCLUDE_DIRS}"
        INTERFACE_COMPILE_OPTIONS "${gflags_CFLAGS_OTHER}"
        INTERFACE_LINK_LIBRARIES "${gflags_LIBRARIES}"
    )
endif()
