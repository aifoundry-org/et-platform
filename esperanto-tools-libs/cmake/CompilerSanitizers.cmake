# given a cmake target
# enables {address, leak, undefined, thread, memory} sanitizers for {gcc, clang, AppleClang} compilers
function(target_add_sanitizers project_name)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        set(SANITIZERS "")

        if (ENABLE_SANITIZER_ADDRESS)
            list(APPEND SANITIZERS "address")
        endif()

        if (ENABLE_SANITIZER_LEAK)
            list(APPEND SANITIZERS "leak")
        endif()

        if (ENABLE_SANITIZER_UNDEFINED_BEHAVIOR)
            list(APPEND SANITIZERS "undefined")
        endif()

        if (ENABLE_SANITIZER_THREAD)
            if ("address" IN_LIST SANITIZERS OR "leak" IN_LIST SANITIZERS)
                message(WARNING "Thread sanitizer does not work with Address and Leak sanitizer enabled")
            else()
                list(APPEND SANITIZERS "thread")
            endif()
        endif()

        if (ENABLE_SANITIZER_MEMORY AND CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
            if ("address" IN_LIST SANITIZERS OR "thread" IN_LIST SANITIZERS OR "leak" IN_LIST SANITIZERS)
                message(WARNING "Memory sanitizer does not work with Address, Thread and Leak sanitizer enabled")
            else()
                list(APPEND SANITIZERS "memory")
            endif()
        endif()

        list(JOIN SANITIZERS "," SANITIZERS_LIST)

    endif()

    if (SANITIZERS_LIST)
        if (NOT "${SANITIZERS_LIST}" STREQUAL "")
            message(STATUS "* Target ${project_name}: Adding sanitizers [${SANITIZERS_LIST}].")
            target_compile_options(${project_name} PUBLIC -fsanitize=${SANITIZERS_LIST})
            target_link_libraries(${project_name} PUBLIC -fsanitize=${SANITIZERS_LIST})
        endif()
    endif()
endfunction()
