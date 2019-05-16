#!/bin/bash

set -e

if [ -z "$1" ]; then
    echo "Usage: get_git_hash.sh <git_folder_path>"
    exit -1
fi

pushd "$1" >> /dev/null
HASH=`git rev-parse HEAD`
popd >> /dev/null

printf "\"%s\"" "${HASH}"
