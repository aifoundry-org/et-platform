#!/bin/bash
# Usage:
#
#   aifoundry/build.sh [<base_os>] [<version>]
BASE_DISTRO=${1:-ubuntu}
BASE_VERSION=${2:-24.04}
TAG="et-platformsw:${BASE_DISTRO}-${BASE_VERSION}"
docker build -t "${TAG}" \
             -f aifoundry/Dockerfile \
             --build-arg BASE_DISTRO="${BASE_DISTRO}" \
             --build-arg BASE_VERSION="${BASE_VERSION}" \
             .
