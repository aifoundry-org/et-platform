find_package(PkgConfig REQUIRED)

pkg_check_modules(libcap REQUIRED libcap)

if(NOT TARGET libcap::libcap)
    add_library(libcap::libcap INTERFACE IMPORTED)
    set_target_properties(libcap::libcap PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${libcap_INCLUDE_DIRS}"
        INTERFACE_COMPILE_OPTIONS "${libcap_CFLAGS_OTHER}"
        INTERFACE_LINK_LIBRARIES "${libcap_LIBRARIES}"
    )
endif()
