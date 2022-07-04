cmake_minimum_required (VERSION 3.17)
project(signedImageFormat VERSION 1.2.0 LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(GNUInstallDirs)

option(ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

add_library(signedImageFormat INTERFACE)
add_library(signedImageFormat::signedImageFormat ALIAS signedImageFormat)
target_include_directories(signedImageFormat
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(signedImageFormat PROPERTIES PUBLIC_HEADER "include/esperanto_signed_image_format/certificate.h;include/esperanto_signed_image_format/certificate_request.h;include/esperanto_signed_image_format/executable_image.h;include/esperanto_signed_image_format/pcie_config_header.h;include/esperanto_signed_image_format/public_key_data.h;include/esperanto_signed_image_format/raw_image.h")

install(TARGETS signedImageFormat
    EXPORT signedImageFormatTargets
    RUNTIME       DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/esperanto_signed_image_format
)

install(EXPORT signedImageFormatTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/signedImageFormat
    NAMESPACE signedImageFormat::
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/signedImageFormatConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/signedImageFormatConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/signedImageFormat
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/signedImageFormatConfigVersion.cmake
    VERSION ${VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/signedImageFormatConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/signedImageFormatConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/signedImageFormat
)

