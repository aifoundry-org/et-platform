#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../../.." && pwd)"

BUILD_DIR="${1:-/tmp/fixed_lpddr_probe_device_build}"
FIXED_BASE_ADDRESS="${2:-0x87fee00000}"
JOBS="${JOBS:-8}"
TOOLCHAIN_FILE="${TOOLCHAIN_FILE:-/opt/et/lib/cmake/riscv64-ec-toolchain.cmake}"

echo "Configuring fixed LPDDR probe build"
echo "  repo root: ${REPO_ROOT}"
echo "  build dir: ${BUILD_DIR}"
echo "  base addr: ${FIXED_BASE_ADDRESS}"
echo "  toolchain: ${TOOLCHAIN_FILE}"

cmake \
  -S "${REPO_ROOT}/gp-sdk/device" \
  -B "${BUILD_DIR}" \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/opt/et \
  -DADDRESS:STRING="${FIXED_BASE_ADDRESS}"

cmake --build "${BUILD_DIR}" --target scp_exec_probe.elf_dbg -j "${JOBS}"

echo
echo "Built fixed-address probe kernel:"
echo "  ${BUILD_DIR}/tests/scp_exec_probe.elf_dbg"
