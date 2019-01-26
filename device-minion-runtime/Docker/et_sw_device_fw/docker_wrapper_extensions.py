# Extensions/modification of the docker
# command specific to this project

#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

"""This script has the docker build/pull/push helpers for bulding the et-sw-software

The et-sw-software image inherits from the et-sw-base image

This script takes for of enforcing this inheritance relationship
"""


import checksumdir
import docker
import grp
import hashlib
import logging
import os
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

sys.path.append(os.path.join(SCRIPT_DIR, '../../'))

import infra_tools.common_docker_images.et_sw_base.docker_wrapper_extensions as parent_image

sys.dont_write_bytecode = True

IMAGE_NAME='et-sw-device-fw'

logger = logging.getLogger(IMAGE_NAME)

# logging.basicConfig(level=logging.DEBUG)


# Default region where this image is stored
region = 'us-west-2'


def base_image_name():
    """Return str with the base image name, without the URL"""
    return IMAGE_NAME


def image_name(region=region):
    return '828856276785.dkr.ecr.{region}.amazonaws.com/{image_name}' \
        ''.format(region=region, image_name=IMAGE_NAME)


def docker_folder():
    """Return path with the image's Docker folder"""
    return os.path.abspath(os.path.join(SCRIPT_DIR, 'Docker'))

3
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
    # The docker folder's hash
    folder_hash = checksumdir.dirhash(docker_folder(), 'sha1')
    logger.debug("Folder Hash: {}".format(folder_hash))
    m.update(bytearray(folder_hash, 'utf-8'))
    # Add the parent image hash as well to capture changes in the parent
    m.update(bytearray(parent_image.image_hash(), 'utf-8'))
    return m.hexdigest()[:10]


def parent_image_exists(docker_client):
    """Return true if a docker image exists"""
    try:
        docker_client.images.get(parent_image.full_image_url_tag())
        return True
    except docker.errors.ImageNotFound:
        return False


def build_image(args, docker_client, image_name, tag, docker_file_path):
    """Build the toolchain docker image

    Args:
      args (Namespace): Passed command line arguments
      docker_client: Docker client
      image_name (str): Name of the docker image
      tag (str): Tag of the image
      docker_file_path (str): Path of the folder file folder, UNUSED
    """
    # Build the parent as well
    if not parent_image_exists(docker_client):
        parent_image.build_image(args, docker_client)
    cmd = "ssh-agent /bin/bash -c \"ssh-add && ssh-add -L && " \
        "docker build --progress=plain  --ssh default --build-arg ET_SW_BASE={base_image}" \
        " --build-arg GIT_SHA={repo_commit} {docker_folder} " \
        " ".format(base_image=parent_image.full_image_url_tag(),
                   repo_commit=repo_head_commit(),
                   docker_folder=docker_folder())
    cmd +=  '-t {}:{}'.format(image_name, tag)
    cmd += "\""
    print(cmd)
    subprocess.check_call(cmd, stdout=sys.stdout,
                          stderr=sys.stderr,
                          shell=True)

def add_docker_args(args, cmd):
    """Modify the arguments passed to docker"""
    docker_group = grp.getgrnam('docker')
    cmd += ['--entrypoint', '/entrypoint.sh',
            # Mount the docker socket because during build time of the
            # docker-image of client#1 we need to communicate with the
            # daemon.
            '-v', '/var/run/docker.sock:/var/run/docker.sock',
            '-e', 'DOCKER_GID={}'.format(docker_group[2]),
    ]
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
