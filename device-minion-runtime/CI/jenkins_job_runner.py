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


""" Helper script to be used for running a jobs regression steps

This is a helper script that consumes a job specification json file,
e.g. maxion-riscvemu.json and executes the different sections
specified in that file: e.g. BUILD_STEPS or TEST_STEPS etc
"""

import argparse
import json
import logging
import subprocess

def run(args):
    """Top level function that runs the build step of a json speciification file.
    """
    with open(args.job_spec_file, 'r') as json_file:
        data = json.load(json_file)
        if args.job_step not in data:
            raise RuntimeError("Unknown job step " + args.job_step)
        steps = data[args.job_step]
        for step in steps:
            logging.info("Executing: " + step)
            subprocess.check_call(step, shell=True)


_LOG_LEVEL_STRINGS = ['CRITICAL', 'ERROR', 'WARNING', 'INFO', 'DEBUG']

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
    parser.add_argument("job_spec_file",
                        help='Path to the job specification json file. The expected '
                        ' format of the file is a dictionary of lists of strings, where '
                        ' the lists contain the commands to execute.')
    parser.add_argument('job_step',
                         help="Build step to execute.")

    args = parser.parse_args()
    logging.basicConfig(level=args.log_level)
    run(args)
