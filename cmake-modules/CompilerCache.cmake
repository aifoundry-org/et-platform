# https://cmake.org/cmake/help/latest/prop_tgt/LANG_COMPILER_LAUNCHER.html?highlight=ccache
if (ENABLE_COMPILER_CACHE)
    find_program(CCACHE_PROGRAM ccache)
    find_program(DISTCC_PROGRAM distcc)
    if (CCACHE_PROGRAM)
        message(STATUS "* ccache found, setting compiler cache.")

        set(ccacheEnv
            CCACHE_CPP2=true
            CCACHE_BASEDIR=${CMAKE_BINARY_DIR}
            CACHE_SLOPPINESS=pch_defines,time_macros
            )
        if (DISTCC_PROGRAM)
            message(STATUS "* distcc found, chaining ccache with distcc.")
            # to chain ccache and distcc, use CCACHE_PREFIX
            list(APPEND ccacheEnv CCACHE_PREFIX=${DISTCC_PROGRAM})
        endif()

        if (CMAKE_GENERATOR MATCHES "Ninja|Makefiles")
            foreach(lang IN ITEMS C CXX OBJC OBJCXX CUDA)
                set(CMAKE_${lang}_COMPILER_LAUNCHER ${CMAKE_COMMAND} -E env ${ccacheEnv} ${CCACHE_PROGRAM} PARENT_SCOPE)
            endforeach()
        elseif (CMAKE_GENERATOR STREQUAL "Xcode")
            # Set Xcode project attributes to route compilation and linking
            # through our scripts
            foreach(lang IN ITEMS C CXX)
                set(launcher ${CMAKE_BINARY_DIR}/launch-${lang})
                file(WRITE ${launcher} "#!/bin/bash\n\n")
                foreach(keyVAL IN LISTS ccacheEnv)
                    file(APPEND ${launcher} "export ${keyVal}\n")
                endforeach()
                file(APPEND ${launcher}
                    "exec \"${CCACHE_PROGRAM}\" \"${CMAKE_${lang}_COMPILER}\" \"$@\"\n"
                    )
                execute_process(COMMAND chmod a+rx ${launcher})
            endforeach()

            set(CMAKE_XCODE_ATTRIBUTE_CC         "${CMAKE_BINARY_DIR}/launch-C")
            set(CMAKE_XCODE_ATTRIBUTE_CXX        "${CMAKE_BINARY_DIR}/launch-CXX")
            set(CMAKE_XCODE_ATTRIBUTE_LD         "${CMAKE_BINARY_DIR}/launch-C")
            set(CMAKE_XCODE_ATTRIBUTE_LDPLUSPLUS "${CMAKE_BINARY_DIR}/launch-CXX")
        else()
        endif()
    elseif(DISTCC_PROGRAM)
        message(STATUS "* distcc found, setting compiler cache.")

        foreach(lang IN ITEMS C CXX OBJC OBJCXX CUDA)
            set(CMAKE_${lang}_COMPILER_LAUNCHER ${DISTCC_PROGRAM})
        endforeach()
    else()
        message(WARNING "no compiler cache was found.")
    endif()
else()
    message(STATUS "* No compiler cache requested.")
endif()