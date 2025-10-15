# Copyright (c) 2025 Ainekko, Co.
# SPDX-License-Identifier: Apache-2.0

install(
  PROGRAMS get_git_hash.py get_git_version.py
  DESTINATION ${CMAKE_INSTALL_BINDIR}/esperanto-fw
  COMPONENT tools)
