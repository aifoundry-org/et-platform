#------------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

# FIXME Set the proper version of the package
set(PACKAGE_VERSION 0.1.0)

configure_file(setup.py.in setup.py @ONLY)
configure_file(rt_logging/__init__.py rt_logging/__init__.py COPYONLY)

if (PYTHON)
  install(CODE "execute_process(
           COMMAND ${PYTHON} -m pip install .
           WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
")
endif()
