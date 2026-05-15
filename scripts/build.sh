#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-"$ROOT_DIR/build"}"
CONFIG="${CONFIG:-Debug}"

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" "$@"
cmake --build "$BUILD_DIR" --config "$CONFIG"
