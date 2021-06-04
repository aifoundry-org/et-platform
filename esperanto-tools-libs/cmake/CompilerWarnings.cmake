# given a cmake target
# sets a list of common warnings for {MSVC, gcc, clang, AppleClang} compilers
function(target_set_project_warnings target_name)
    set(MSVC_WARNINGS
        /W4 # Baseline reasonable warnings
        /w14242 # 'identifier': conversion from 'type1' to 'type1', possible loss of data
        /w14254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
        /w14263 # 'function': member function does not override any base class virtual member function
        /w14265 # 'classname': class has virtual functions, but destructor is not virtual instances of this class may not
        # be destructed correctly
        /w14287 # 'operator': unsigned/negative constant mismatch
        /we4289 # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside
        # the for-loop scope
        /w14296 # 'operator': expression is always 'boolean_value'
        /w14311 # 'variable': pointer truncation from 'type1' to 'type2'
        /w14545 # expression before comma evaluates to a function which is missing an argument list
        /w14546 # function call before comma missing argument list
        /w14547 # 'operator': operator before comma has no effect; expected operator with side-effect
        /w14549 # 'operator': operator before comma has no effect; did you intend 'operator'?
        /w14555 # expression has no effect; expected expression with side- effect
        /w14619 # pragma warning: there is no warning number 'number'
        /w14640 # Enable warning on thread un-safe static member initialization
        /w14826 # Conversion from 'type1' to 'type_2' is sign-extended. This may cause unexpected runtime behavior.
        /w14905 # wide string literal cast to 'LPSTR'
        /w14906 # string literal cast to 'LPWSTR'
        /w14928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
        /permissive- # standards conformance mode for MSVC compiler.
    )

    # Separating flags between C/C++ common falgs and C or C++ exclusive flags

    set(CLANG_COMMON_WARNINGS
        -Wall
        -Wextra # reasonable and standard
        -Wshadow # warn the user if a variable declaration shadows one from a parent context
        -Wnull-dereference # warn if a null dereference is detected
        -Wcast-align # warn for potential performance problem casts
        -Wunused # warn on anything being unused
        -Wpedantic # warn if non-standard C/C++ is used
        -Wconversion # warn on type conversions that may lose data
        -Wsign-conversion # warn on sign conversions
        -Wdouble-promotion # warn if float is implicit promoted to double
        -Wformat=2 # warn on security issues around functions that format output (ie printf)
        -Wpointer-arith
        -Wundef
        -Wcast-qual
        -Wcast-align
        -Wconversion
        -Wmissing-declarations
    )

    if (ENABLE_WARNINGS_AS_ERRORS)
        message(STATUS "* Target ${target_name}: Treating warnings as errors.")
        set(CLANG_COMMON_WARNINGS ${CLANG_COMMON_WARNINGS} -Werror)
        set(MSVC_WARNINGS ${MSVC_WARNINGS} /WX)
    endif()

    set(CLANG_C_WARNINGS
        ${CLANG_COMMON_WARNINGS}
    )
    set(CLANG_CXX_WARNINGS
        ${CLANG_COMMON_WARNINGS}
        -Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual destructor. This helps
        # catch hard to track down memory errors
        -Wold-style-cast # warn for c-style casts
        -Woverloaded-virtual # warn if you overload (not override) a virtual function
    )

    set(GCC_COMMON_WARNINGS
        -Wmisleading-indentation # warn if indentation implies blocks where blocks do not exist
        -Wduplicated-cond # warn if if / else chain has duplicated conditions
        -Wduplicated-branches # warn if if / else branches have duplicated code
        -Wlogical-op # warn about logical operations being used where bitwise were probably wanted
    )

    set(GCC_C_WARNINGS
        ${CLANG_C_WARNINGS}
        ${GCC_COMMON_WARNINGS}
    )

    set(GCC_CXX_WARNINGS
        ${CLANG_CXX_WARNINGS}
        ${GCC_COMMON_WARNINGS}
        -Wuseless-cast # warn if you perform a cast to the same type
    )
    if (NOT (MSVC OR CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
        message(AUTHOR_WARNING "No compiler warnings set for '${CMAKE_CXX_COMPILER_ID}' compiler.")
    else()
        message(STATUS "* Target ${target_name}: Adding warning flags.")
    endif()

    target_compile_options(${target_name} PRIVATE
        $<$<C_COMPILER_ID:Clang>:${CLANG_C_WARNINGS}>
        $<$<C_COMPILER_ID:AppleClang>:${CLANG_C_WARNINGS}>
        $<$<C_COMPILER_ID:GNU>:${GCC_C_WARNINGS}>

        $<$<CXX_COMPILER_ID:MSVC>:${MSVC_WARNINGS}>
        $<$<CXX_COMPILER_ID:Clang>:${CLANG_CXX_WARNINGS}>
        $<$<CXX_COMPILER_ID:AppleClang>:${CLANG_CXX_WARNINGS}>
        $<$<CXX_COMPILER_ID:GNU>:${GCC_CXX_WARNINGS}>
    )

endfunction()