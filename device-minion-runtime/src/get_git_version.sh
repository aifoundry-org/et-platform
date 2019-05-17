#!/bin/bash

set -e

if [ -z "$1" ]; then
    echo "Usage: get_git_hash.sh <git_folder_path>"
    exit -1
fi

pushd "$1" >> /dev/null
VERSION=`git describe --dirty --always --tags`
popd >> /dev/null

if [[ "$VERSION" == *-dirty ]]
then
    DATE=`date --rfc-3339=seconds`
    printf "\"%s %s\"" "${VERSION}" "${DATE}"
else
    printf "\"%s\"" "${VERSION}"
fi
