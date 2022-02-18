add_library(common 
    src/constant_memory_compare.c
    src/sp_otp.c
)
add_library(esperantoBootLoader::common ALIAS common)
target_include_directories(common
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(common 
    PUBLIC
        etsoc_hal::etsoc_hal
        signedImageFormat::signedImageFormat
        esperanto-flash-tool::headers
    PRIVATE
        et-common-libs::sp-bl2
)
