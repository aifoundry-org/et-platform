#!/usr/bin/env python3

#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

"""
Top level helper script for manually launching jobs
in Jenkins


Status: The script is experimental and ugly

"""

import sys
# Do not generate bytecode for this script
sys.dont_write_bytecode = True

import argparse
import argcomplete
import jenkins
import json
import logging
import os
import subprocess
import time

# This so that the script can remain self-contained
REPOROOT=os.path.abspath(os.path.join(os.path.realpath(__file__), "../.."))
sys.path.append(REPOROOT)
import infra_tools.jenkins_helpers as jenkins_helpers

_LOG_LEVEL_STRINGS = ['CRITICAL', 'ERROR', 'WARNING', 'INFO', 'DEBUG']

JENKINS_SERVER = 'http://sc-jenkins01.esperanto.ai:8080'


class JenkinsParams:
    """Generate the parameters for the jobs using other groovy files

    Attributes:
    job_name (str): Name of the job
    """
    def __init__(self, job_name):
        self.job_name = job_name

    def gen_params(self, branch, **kwargs):
        """Generate the job's parameters"""
        return {
            'BRANCH': branch,
            }


class JenkinsfileParams:
    """Generate the parameters for the jobs using Jenkinsfile.groovy

    Attributes:
    job_name (str): Name of the jobOB
    json_config_file (str): Path of the json confiugration file
       for the job.
    """
    def __init__(self, job_name):
        self.job_name = job_name
        self.json_config_file = job_name + '.json'

    def gen_params(self, branch, **kwargs):
        """Generate the job's parameters"""
        return {
            'BRANCH': branch,
            'GIT_STEPS': './CI/jenkins_job_runner.py ./CI/{} GIT_STEPS'
            ''.format(self.json_config_file),
            'BUILD_STEPS': './CI/jenkins_job_runner.py ./CI/{} BUILD_STEPS'
            ''.format(self.json_config_file),
            'TEST_STEPS': './CI/jenkins_job_runner.py ./CI/{} TEST_STEPS'
            ''.format(self.json_config_file)
            }


JOB_DESCRIPTION = {
    'docker-image-builder': JenkinsfileParams('Software/device-fw/docker-image-builder'),
    'checkin-regression': JenkinsfileParams('Software/device-fw/checkin-regression'),
    "Software/sw-platform/checkin-top-level" : JenkinsfileParams("Software/sw-platform/checkin-top-level"),
}

def run_subcommand(args):
    """Run a subcommand

    Args:
       args: List of command line arguments

    Returns:
       string: String output of the command

    Raises:
       CalledProcessError in case the command fails.
    """
    return subprocess.check_output(args).decode("utf-8").strip()


def run(args, unknown_args):
    """Top level function that triggers a job in the jenkins server

    Args:
       args (Namespace): Namespace object returned after parsing the
         command line arguments
       unknown_args ([]): List of unknown args the parser did not recognize
         used only when integrated with maxion.py
    """
    branch_name = args.branch
    if args.commit_passed:
        check_job_params = json.loads(args.check_job_params) if args.check_job_params else None
        if len(args.job) > 1:
            print("Can handle only one job at a time")
            sys.exit(-10)
        job_name = args.job[0]
        res, url = jenkins_helpers.job_passed_for_commit(JENKINS_SERVER,
                                                         JOB_DESCRIPTION[job_name].job_name,
                                                         args.commit_passed,
                                                         check_job_params)
        if res > 0:
            logging.info("Previous job: " + url)
            sys.exit(0)
        else:
            sys.exit(1)

    for job in args.job:
        if not branch_name:
            branch_name = run_subcommand(['git', 'rev-parse', '--abbrev-ref', 'HEAD'])
        job_params_obj = JOB_DESCRIPTION[job]
        job_params = job_params_obj.gen_params(branch_name)
        jenkins_helpers.launch_job(JENKINS_SERVER, job_params_obj.job_name, job_params)


def _log_level_string_to_int(log_level_string):
    if not log_level_string in _LOG_LEVEL_STRINGS:
        message = 'invalid choice: {0} (choose from {1})'.format(log_level_string, _LOG_LEVEL_STRINGS)
        raise argparse.ArgumentTypeError(message)
    log_level_int = getattr(logging, log_level_string, logging.INFO)
    # check the logging log_level_choices have not changed from our expected values
    assert isinstance(log_level_int, int)
    return log_level_int


def register_command(parser):
    """Register the command line arguments

    Args:
       parser (argparse.ArgumentParser): Command line argument parser
    """
    parser.add_argument('--log-level',
                        default='INFO',
                        dest='log_level',
                        type=_log_level_string_to_int,
                        nargs='?',
                        help='Set the logging output level. {0}'.format(_LOG_LEVEL_STRINGS))
    parser.add_argument("--branch",
                        default=None,
                        help='Name of the git branch to run the Jenkins job with, if not'
                        'specified then the current checkout branch will be running. We '
                        'assume that the user has pushed the latest version o the branch '
                        'to the repo')
    parser.add_argument("--commit-passed",
                        default=None,
                        help="Return 0 or 1 if the commit passed for a specific job")
    parser.add_argument("--check-job-params",
                        default=None,
                        help="String of key value pairs of the parameters names/values to "
                        "check against. The format of the string is json")
    parser.add_argument('job',
                        nargs="+",
                        choices=list(JOB_DESCRIPTION.keys()),
                        metavar=" ".join(list(JOB_DESCRIPTION.keys())),
                        help="Name of the job to run")
    parser.set_defaults(func=run)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    register_command(parser)

    argcomplete.autocomplete(parser)
    args = parser.parse_args()
    logging.basicConfig(level=args.log_level)
    run(args, [])
