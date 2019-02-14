#!/usr/bin/env python3

""" Code generator
"""

import sys
sys.dont_write_bytecode = True

import argparse
import json
import logging
import os

_LOG_LEVEL_STRINGS = ['CRITICAL', 'ERROR', 'WARNING', 'INFO', 'DEBUG']

def run(args, unknown_args):
    """Main Execution function"""
    with open(args.output_file, 'w') as output:
        json.dump("test", output)


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
    parser.add_argument('-o', '--output-file',
                        help="Output File")
    parser.add_argument('input_file',
                        help='ET_API file to parse')

    args, unknown_args = parser.parse_known_args()
    logging.basicConfig(level=args.log_level)
    run(args, unknown_args)
