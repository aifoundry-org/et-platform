find_package(PkgConfig REQUIRED)

pkg_check_modules(PC_LIBCAP REQUIRED libcap)

if(NOT TARGET libcap::libcap)
    add_library(libcap::libcap INTERFACE IMPORTED)
    set_target_properties(libcap::libcap PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${PC_LIBCAP_INCLUDE_DIRS}"
        INTERFACE_COMPILE_OPTIONS "${PC_LIBCAP_CFLAGS_OTHER}"
        INTERFACE_LINK_LIBRARIES "${PC_LIBCAP_LIBRARIES}"
    )
endif()
