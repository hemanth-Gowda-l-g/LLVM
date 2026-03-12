#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
EXAMPLES_DIR="$ROOT_DIR/examples"
PLUGIN_SO="$BUILD_DIR/libAMDOptimizer.so"

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing required command: $1" >&2
    return 1
  fi
}

echo "[1/4] Checking required tools..."
require_cmd cmake
require_cmd clang
require_cmd opt

echo "[2/4] Configuring and building plugin..."
cmake -S "$ROOT_DIR" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR"

if [[ ! -f "$PLUGIN_SO" ]]; then
  echo "Expected plugin not found: $PLUGIN_SO" >&2
  exit 1
fi

echo "[3/4] Running optimizer on examples..."
shopt -s nullglob
for src in "$EXAMPLES_DIR"/*.c; do
  name="$(basename "$src" .c)"
  ir_in="$BUILD_DIR/${name}.ll"
  ir_out="$BUILD_DIR/opt_${name}.ll"

  echo "  - $name"
  clang -O0 -S -emit-llvm "$src" -o "$ir_in"
  opt -load-pass-plugin "$PLUGIN_SO" -passes=amd-opt -S "$ir_in" -o "$ir_out"
done

echo "[4/4] Done. Generated optimized IR files:"
ls -1 "$BUILD_DIR"/opt_*.ll
