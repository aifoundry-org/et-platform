#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
find_package(Sphinx REQUIRED)

if(NOT DEFINED SPHINX_THEME)
    set(SPHINX_THEME default)
endif()

if(NOT DEFINED SPHINX_THEME_DIR)
    set(SPHINX_THEME_DIR)
endif()

# configured documentation tools and intermediate build results
set(BINARY_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/_build")

# Sphinx cache with pickled ReST documents
set(SPHINX_CACHE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_doctrees")

# HTML output directory
set(SPHINX_HTML_DIR "${CMAKE_CURRENT_BINARY_DIR}/html")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
    @ONLY)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in"
    "${CMAKE_CURRENT_BINARY_DIR}/conf.py"
    @ONLY)

file(GLOB_RECURSE DOC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}} *.rst *.png)

set(DOC_FILES_DST "")
foreach(DOC_FILE ${DOC_FILES})
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${DOC_FILE}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${DOC_FILE}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_FILE}
    )
  list(APPEND DOC_FILES_DST ${CMAKE_CURRENT_BINARY_DIR}/${DOC_FILE})
endforeach()


add_custom_target(doc-html
    ${SPHINX_EXECUTABLE}
        -q -b html
        -d "${SPHINX_CACHE_DIR}"
        -j auto
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${SPHINX_HTML_DIR}"
    COMMENT "Building HTML documentation with Sphinx"
    DEPENDS Doxyfile.in
            conf.py.in
            ${DOC_FILES_DST}
  )

install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
    DESTINATION ${DOCUMENTATION_INSTALL_DIR}
    COMPONENT documentation
    EXCLUDE_FROM_ALL
    )

set(SPHINX_PDF_DIR "${CMAKE_CURRENT_BINARY_DIR}/latex")

add_custom_target(doc-pdf
  ${SPHINX_EXECUTABLE}
        -M latexpdf
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${SPHINX_PDF_DIR}"
  COMMENT "Building PDF documentation with Sphinx"
  DEPENDS Doxyfile.in
          conf.py.in
          ${DOC_FILES_DST}
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/latex/latex/esperantodevicemanagement.pdf
    DESTINATION ${DOCUMENTATION_INSTALL_DIR}
    COMPONENT documentation
    EXCLUDE_FROM_ALL
  )

add_custom_target(doc-all
  DEPENDS doc-html doc-pdf
  )

# Temporary alias to preserve the existing interface
add_custom_target(doc
  DEPENDS doc-all
)

add_custom_target(doc-install
  COMMAND ${CMAKE_COMMAND} -DCOMPONENT="documentation" -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_install.cmake
  DEPENDS doc-all
)
