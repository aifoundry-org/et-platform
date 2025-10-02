# given a cmake target
# setups a set of options that may affect compilation flags
function(target_set_project_options project_name)
    # set the c++ standard
    if (CONAN_CMAKE_CXX_STANDARD)
        message(STATUS "* Target ${project_name}: Deducing C++ standard (${CONAN_CMAKE_CXX_STANDARD}) from Conan compiler.cppstd setting")
        set_property(TARGET ${project_name} PROPERTY CXX_STANDARD ${CONAN_CMAKE_CXX_STANDARD})
    else()
        message(STATUS "* Target ${project_name}: Setting default C++ standard 17")
        target_compile_features(${project_name} PUBLIC cxx_std_17)
    endif()

    # ##############################################################################

    # handle rrti using targets
    if (ENABLE_RTTI)
        message(STATUS "* Target ${project_name}: RTTI is enabled.")

        set(MSVC_RTTI_ON_FLAG /GR)
        set(GCC_RTTI_ON_FLAG -frtti)

        target_compile_options(${project_name}
            INTERFACE $<$<CXX_COMPILER_ID:MSVC>:${MSVC_RTTI_ON_FLAG}>
            $<$<CXX_COMPILER_ID:Clang>:${GCC_RTTI_ON_FLAG}>
            $<$<CXX_COMPILER_ID:AppleClang>:${GCC_RTTI_ON_FLAG}>
            $<$<CXX_COMPILER_ID:GNU>:${GCC_RTTI_ON_FLAG}>)
    else ()
        message(STATUS "* Target ${project_name}: RTTI is disabled.")

        set(MSVC_RTTI_OFF_FLAG /GR-)
        set(GCC_RTTI_OFF_FLAG -fno-rtti)

        if (MSVC)
            string(REGEX REPLACE "/GR" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        endif()

        target_compile_options(${project_name}
            INTERFACE $<$<CXX_COMPILER_ID:MSVC>:${MSVC_RTTI_OFF_FLAG}>
            $<$<CXX_COMPILER_ID:Clang>:${GCC_RTTI_OFF_FLAG}>
            $<$<CXX_COMPILER_ID:AppleClang>:${GCC_RTTI_OFF_FLAG}>
            $<$<CXX_COMPILER_ID:GNU>:${GCC_RTTI_OFF_FLAG}>)
    endif()

    # handle exceptions flag using targets
    if (NOT ENABLE_EXCEPTIONS)
        message(STATUS "* Target ${project_name}: Exceptions have been disabled. Any operation that would "
            "throw an exception will result in a call to std::abort() instead.")

        if (MSVC)
            string(REGEX REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        endif()

        set(MSVC_EXCEPTIONS_FLAG /EHs-c-)
        set(MSVC_EXCEPTIONS_DEFINITIONS _HAS_EXCEPTIONS=0)

        set(GCC_EXCEPTIONS_FLAG -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables)

        target_compile_definitions(${project_name}
            INTERFACE $<$<CXX_COMPILER_ID:MSVC>:${MSVC_EXCEPTIONS_DEFINITIONS}>)
        target_compile_options(${project_name}
            INTERFACE $<$<CXX_COMPILER_ID:MSVC>:${MSVC_EXCEPTIONS_FLAG}>
            $<$<CXX_COMPILER_ID:Clang>:${GCC_EXCEPTIONS_FLAG}>
            $<$<CXX_COMPILER_ID:AppleClang>:${GCC_EXCEPTIONS_FLAG}>
            $<$<CXX_COMPILER_ID:GNU>:${GCC_EXCEPTIONS_FLAG}>)
    else()
        message(STATUS "* Target ${project_name}: Exceptions are enabled.")
    endif()

    # Very basic PCH
    # This sets a global PCH parameter, each project will build its own PCH, which
    # is a good idea if any #define's change
    if (ENABLE_PCH)
        message(STATUS "* Target ${project_name}: Generating PCH.")
        target_precompile_headers(${project_name}
            INTERFACE
            <vector>
            <string>
            <map>
            <utility>
            )
    endif()
endfunction()