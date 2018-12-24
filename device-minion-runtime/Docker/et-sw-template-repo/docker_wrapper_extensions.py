# Extensions/modification of the docker
# command specific to this project

#------------------------------------------------------------------------------
# Copyright (C) 2018, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------


import checksumdir
import hashlib
import logging
import os
import subprocess
import sys

IMAGE_NAME='et-sw-template-repo'

logger = logging.getLogger(IMAGE_NAME)

# logging.basicConfig(level=logging.DEBUG)

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# Default region where this image is stored
region = 'us-west-2'

def image_name(region=region):
    return '828856276785.dkr.ecr.{region}.amazonaws.com/{image_name}' \
        ''.format(region=region, image_name=IMAGE_NAME)

def repo_head_commit():
    """Return the HEAD commit SHA"""
    return subprocess.check_output(['git', 'rev-parse',
                                    'HEAD']).decode('utf-8').strip()

def image_hash():
    """Return list of files and folder to generate the image hash

    Projects could have file dependencies that are outside the Dockerfile
    and entrypoint.sh script. Those additional files need to be taken
    into account when computing the unique HASH tag we will use for
    tagging the image

    Returns:
       string: 10 digits of the hash of the files comprizing the image
    """
    m = hashlib.sha256()
    docker_folder = os.path.join(SCRIPT_DIR, 'Docker')
    # The docker folder's hash
    folder_hash = checksumdir.dirhash(docker_folder, 'sha1')
    logger.debug("Folder Hash: {}".format(folder_hash))
    m.update(bytearray(folder_hash, 'utf-8'))
    return m.hexdigest()[:10]


def build_image(args, docker_client, image_name, tag, docker_file_path):
    """Build the toolchain docker image

    Args:
      args (Namespace): Passed command line arguments
      docker_client: Docker client
      image_name (str): Name of the docker image
      tag (str): Tag of the image
      docker_file_path (str): Path of the folder file folder
    """
    riscvtools_dir = os.path.join(SCRIPT_DIR, '../../riscv-tools')
    cmd = [
        'docker', 'build',
        '--build-arg', 'GIT_SHA=' + repo_head_commit(),
        docker_file_path,
        ]
    # Optionally squash the image to eliminate any unecessary
    # intermediate layers. Side-effects we will be that we will
    # not be re-using any earlier layers and rebuilds the image
    # from scratch.
    if getattr(args, 'squash', False):
        cmd += ['--squash']
    cmd += [
        '-t', "{}:{}".format(image_name, tag)
        ]
    print(cmd)
    subprocess.check_call(cmd, stdout=sys.stdout,
                            stderr=sys.stderr)

def add_docker_args(args, cmd):
    """Modify the arguments passed to docker"""
    cmd += ['--entrypoint', '/entrypoint.sh']
    return cmd

def add_entrypoint_args(args, cmd):
    """Modify the arguments passed to the docker entrypoint"""
    pass


def register_subparser(parser, docker_default_arguments):
    """Register a subparser for this image's cmd-line arguments"""
    subparser = parser.add_parser(IMAGE_NAME,
                                  help='Interact with the ' + IMAGE_NAME + \
                                  ' image.')
    for i, args in docker_default_arguments:
        subparser.add_argument(i, **args)
