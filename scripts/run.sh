#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-"$ROOT_DIR/build"}"
CONFIG="${CONFIG:-Debug}"

"$ROOT_DIR/scripts/build.sh" "$@"

if [[ -x "$BUILD_DIR/$CONFIG/memory-playground.exe" ]]; then
    exec "$BUILD_DIR/$CONFIG/memory-playground.exe"
elif [[ -x "$BUILD_DIR/memory-playground" ]]; then
    exec "$BUILD_DIR/memory-playground"
elif [[ -x "$BUILD_DIR/memory-playground.exe" ]]; then
    exec "$BUILD_DIR/memory-playground.exe"
fi

echo "memory-playground executable was not found. Is raylib available to CMake?" >&2
exit 1
