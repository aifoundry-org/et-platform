#!/bin/bash

set -eu

die() { echo "$*" >&2; exit 1; }

[ $# -eq 1 ] || die "Usage: $0 <lib>"

strip=$(nm --defined-only -A $1 \
    | grep -e emu7ISysEmu -e emu9Exception -e emu13SysEmuOptions \
    | awk 'BEGIN{printf "strip"} {printf " --keep-symbol="$3} END{printf "\n"}')

$strip $1
