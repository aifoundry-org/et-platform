Software Project Template
=========================

This is a template project that provides/demonstrates how to:

*  Build our project using CMake, example code structure
*  Generate documentation using Doxygen
*  Use Docker for building and running our project
*  Example CI setup in Jenkins, it builds in AWS and saves artifacts
   in S3 bucket.
*  Example clang-format and clang-tidy setups


##  Entrypoint command

To facilitate interaction of the user with the underlying scripting
infrastructure an entrypoint script is provided with the same nameas the
repo. In this case tis called:

    template-repo.py

Try the `--help` to see all available subcommands to this entrypoint script.

The entrypoint script will provide support to:

*  Interrupt with Docker
*  Launch manually any regressions to Jenkins
*  Any other functionality we want to automate for the user

## Interacting with Docker

See the available subcommands doing:

    template-repo.py docker -h

To instantiate a container and get prompt try:

    ./template-repo.py docker prompt et-sw-template-repo

The `prompt` subcommands instantiates a container and mounts inside it
the source code. It also creates a user with the same UID/GID as the one
instatiating the container, so that any files touched by the container
in the mounted source code have the right permissions.

The prompt has other options, e.g. mounting the user's home directory
or other volumes etc.

### Execute a command inside the container and exit

To execute run somehting directly inside the container you can do:

   ./template-repo.py et-sw-template-repo echo test
INFO:root:Running cmd: docker run --rm --hostname Docker -e UID=1000 -e GID=1000 -e USERNAME=ubuntu -e SRC_DIR=/mnt/lala/Project/SW/template-repo -e CREATE_HOME=1 -v /mnt/lala/Project/SW/template-repo:/mnt/lala/Project/SW/template-repo --entrypoint /entrypoint.sh 828856276785.dkr.ecr.us-west-2.amazonaws.com/et-sw-template-repo:fe2a657e2e echo test
test

In the above example we instantiated the container and run `echo test` inside it.
The `et-sw-template-repo` subcommand has the property that any arguments that it does not
recognize to pass them through to the container

## Build instructions

To build the repo manually, instantiate a container and drop into a prompt as shown above and
run:

    mkdir install
    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=../install ../
    make all -j $(nproc)
    make install


## Jenkins regressions

The logic for the jenkins regressions is implemented in the CI folder.
There you will file the `Jenkinsfile` that implements a jenkins pipeline
using groovy. The Jenkinsfile implements a basic template that can be
applicate to most jobs: Download the docker image, update the git-repo
build the code, run the tests, upload the results in a S3 bucket. The
jenkins regression is expected to run on AWS.

The same execution steps of the regression job we have added so far
can be foud in `CI/Software/template-repo.json`

The same steps can be executed by the user in order to debug the job locally

TODO: Enable running the job locally by extending the entrypoint script.


## Clang tools

We are using:

* clang-format for auto-formating the code
* clang-tidy for running a linter


### Code auto-formatting using clang-format

Taken from https://github.com/andrewseidl/githook-clang-format

TODO register the hook by default

Added https://llvm.org/svn/llvm-project/cfe/trunk/tools/clang-format/git-clang-format
in order to automatically call clang format.

Run ```git clang-format``` to format the code
