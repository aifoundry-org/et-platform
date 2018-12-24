#!/usr/bin/env python3

#------------------------------------------------------------------------------
# Copyright (C) 2018, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

import argparse
import argcomplete
import logging
import os
import sys

sys.path.append('CI')

import infra_tools.docker_helpers as docker_helpers
import CI.jenkins_jobs as jenkins_jobs

# we do not want bytecode artifacts inside the source tree
sys.dont_write_bytecode = True

_LOG_LEVEL_STRINGS = ['CRITICAL', 'ERROR', 'WARNING', 'INFO', 'DEBUG']

PROJECTS = ['et-sw-template-repo']

WRAPPER_EXTENSIONS = {}
for f in PROJECTS:
    mod =  __import__('Docker.' + f +  '.docker_wrapper_extensions')
    WRAPPER_EXTENSIONS[f] = getattr(getattr(mod, f),'docker_wrapper_extensions')


def _log_level_string_to_int(log_level_string):
    if not log_level_string in _LOG_LEVEL_STRINGS:
        message = 'invalid choice: {0} (choose from {1})'.format(log_level_string, _LOG_LEVEL_STRINGS)
        raise argparse.ArgumentTypeError(message)
    log_level_int = getattr(logging, log_level_string, logging.INFO)
    # check the logging log_level_choices have not changed from our expected values
    assert isinstance(log_level_int, int)
    return log_level_int


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--log-level',
                        default='INFO',
                        dest='log_level',
                        type=_log_level_string_to_int,
                        nargs='?',
                        help='Set the logging output level. {0}'.format(_LOG_LEVEL_STRINGS))

    subparsers = docker_helpers.register_commands(parser, WRAPPER_EXTENSIONS)

    subparser = subparsers.add_parser("jenkins-jobs",
                                      help="Trigger jenkins job")
    jenkins_jobs.register_command(subparser)

    argcomplete.autocomplete(parser)
    args, unknown_args = parser.parse_known_args()
    logging.basicConfig(level=args.log_level)

    args.func(args, unknown_args)
