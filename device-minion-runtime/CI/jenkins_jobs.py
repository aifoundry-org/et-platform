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

import argparse
import argcomplete
import jenkins
import logging
import sys
import subprocess
import time

# Do not generate bytecode for this script
sys.dont_write_bytecode = True

_LOG_LEVEL_STRINGS = ['CRITICAL', 'ERROR', 'WARNING', 'INFO', 'DEBUG']

# List of parameters that the different Jenkinsfiles expect


JENKINS_SERVER = 'http://sc-jenkins01.esperanto.ai:8080'


def parent_job_info(child_job_name, child_build_id, timeout=10):
    """Return the upstream job information if it exists.

    Args:
      child_job_name (str): Name of the job
      child_build_id (int): Id of the build
      timeout (int): jenkins server connection timeout

    Returns:
      {'build_name': str, 'build_id': int}: Dictonary with the information of the
        build it it exists empty otherwise.
    """
    server = jenkins.Jenkins(JENKINS_SERVER, timeout=timeout)
    build = server.get_build_info(child_job_name, child_build_id)
    action0 = build['actions'][0]
    if 'causes' not in action0:
        return {'build_name': "None",
                'build_id': -1}
    parent_job_info = action0['causes'][0]
    return {'build_name': parent_job_info['upstreamProject'],
            'build_id': parent_job_info['upstreamBuild']}


class JenkinsfileParams:
    """Generate the parameters for the jobs using Jenkinsfile

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
         used only when integrated with device-firmware.py
    """
    server = jenkins.Jenkins(JENKINS_SERVER)
    branch_name = args.branch
    for job in args.job:
        if not branch_name:
            branch_name = run_subcommand(['git', 'rev-parse', '--abbrev-ref', 'HEAD'])
        job_params_obj = JOB_DESCRIPTION[job]
        job_params = job_params_obj.gen_params(branch_name)
        queue_item_num = server.build_job(job_params_obj.job_name, job_params)
        logging.debug("JobID: {}".format(queue_item_num))
        retries = 0
        executable = None
        while retries < 10 and not executable:
            queue_item = server.get_queue_item(queue_item_num)
            executable = queue_item.get('executable', None)
            if executable:
                break;
            time.sleep(10 + retries * 10)
            retries = retries + 1
            logging.info("Waiting for queueed item: {} url: {}".format(queue_item_num,
                                                                       queue_item['url']))
        if executable:
            print("Started job {}\n\t URL: {}".format(job, executable['url']))
        else:
            logging.error("No Job found for queued item: {}".format(queue_item_num))


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
