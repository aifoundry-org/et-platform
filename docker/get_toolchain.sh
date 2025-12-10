#!/bin/bash

set -e

[ -n "$DEBUG" ] && set -x

help() {
    echo "Usage: $0 --repo <repo> --install-dir <install_dir> --base-distro <base_distro> --distro-version <distro_version> --arch <arch> --abi <abi>"
    echo ""
    echo "Options:"
    echo "  --force           Force rebuild even if an existing build is found"
    echo "  --repo            GitHub repository in the format 'owner/repo' (e.g., 'aifoundry-org/riscv-gnu-toolchain')"
    echo "  --install-dir     Directory where the toolchain should be installed"
    echo "  --base-distro     Base distribution (e.g., 'ubuntu', 'debian')"
    echo "  --distro-version  Version of the base distribution (e.g., '20.04', '22.04')"
    echo "  --arch            RISC-V architecture (e.g., 'rv64imfc')"
    echo "  --abi             RISC-V ABI (e.g., 'lp64f')"
    echo "  --help            Show this help message"
}

# script that either downloads an existing riscv-gnu-toolchain build,
# or, if it cannot find the specific one, builds it from source.
FORCE_REBUILD="false"
INSTALL_DEPS="false"
while [[ $# -gt 0 ]]; do
    case $1 in
        --force) FORCE_REBUILD="$2"; shift 2;;
        --repo) repo="$2"; shift 2 ;;
        --install-dir) install_dir="$2"; shift 2 ;;
        --base-distro) base_distro="$2"; shift 2 ;;
        --distro-version) distro_version="$2"; shift 2 ;;
        --arch) arch="$2"; shift 2 ;;
        --abi) abi="$2"; shift 2 ;;
        --install-deps) INSTALL_DEPS="$2"; shift 2 ;;
        --help) help; exit 1 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

# see if the one we want already exists as a release, in which case we can just pull it down
if [ -z "$repo" ] || [ -z "$install_dir" ] || [ -z "$base_distro" ] || [ -z "$distro_version" ] || [ -z "$arch" ] || [ -z "$abi" ]; then
    echo "Error: Missing required arguments."
    help
    exit 1
fi

if [ "$FORCE_REBUILD" = "true" ]; then
    echo "Force rebuild specified, skipping download of existing release artifact."
else
    if [ "$INSTALL_DEPS" = "false" ]; then
        echo "Skipping installation of dependencies for downloading existing release artifact as per user request."
    else
        echo "Installing dependencies for downloading existing release artifact."
        case "$base_distro" in
            ubuntu|debian)
                export DEBIAN_FRONTEND=noninteractive
                apt-get update && apt-get install -y \
                    curl \
                    jq \
                    xz-utils
                ;;
            *)
                echo "Unsupported base distro: $base_distro"
                exit 1
                ;;
        esac
    fi

    artifact_name="riscv64-elf-${base_distro}-${distro_version}-gcc-stripped.tar.xz"

    downloadurl=$(curl -s https://api.github.com/repos/${repo}/releases/latest | jq -r '.assets[] | select(.name=="'"${artifact_name}"'") | .browser_download_url')

    if [ -n "$downloadurl" ]; then
        echo "Found existing release artifact at ${downloadurl}, downloading and extracting to ${install_dir}"
        mkdir -p ${install_dir}
        curl -L ${downloadurl} | tar --strip-components=1 -xJ -C ${install_dir}
        exit 0
    fi
fi

if [ "$INSTALL_DEPS" = "false" ]; then
    echo "Skipping installation of dependencies for building riscv-gnu-toolchain as per user request."
else
    echo "Installing dependencies for building riscv-gnu-toolchain."
    case "$base_distro" in
        ubuntu|debian)
            export DEBIAN_FRONTEND=noninteractive
            apt-get update && apt-get install -y \
                autoconf \
                automake \
                autotools-dev \
                curl \
                python3 \
                python3-pip \
                python3-tomli \
                libmpc-dev \
                libmpfr-dev \
                libgmp-dev \
                gawk \
                build-essential \
                bison \
                flex \
                texinfo \
                gperf \
                libtool \
                patchutils \
                bc \
                zlib1g-dev \
                libexpat-dev \
                dos2unix \
                ninja-build \
                git \
                cmake \
                libglib2.0-dev \
                libslirp-dev
            ;;
        *)
            echo "Unsupported base distro: $base_distro"
            exit 1
            ;;
    esac
fi

mkdir -p ${install_dir} && chmod 755 ${install_dir}

tmpdir=$(mktemp -d)
cd ${tmpdir}
git clone https://github.com/${repo} riscv-gnu-toolchain
cd riscv-gnu-toolchain
./configure \
    --prefix=${install_dir} \
    --with-arch=${arch} \
    --with-abi=${abi} \
    --with-languages=c,c++ \
    --with-cmodel=medany
make -j$(nproc)
cd /tmp
rm -rf ${tmpdir}
