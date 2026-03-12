#!/bin/bash
set -e

cd /home/hemanth/new_project

CLANG=clang-15
OPT=opt-15
SO=build/libAMDOptimizer.so

echo "=== Rebuilding plugin ==="
cmake -S . -B build -DLLVM_DIR=/usr/lib/llvm-15/cmake 2>&1 | tail -3
cmake --build build 2>&1 | tail -3

echo ""
echo "=== Running optimizer on all examples ==="

for src in examples/*.c; do
    name=$(basename "$src" .c)
    ir="build/${name}.ll"
    out="build/opt_${name}.ll"

    echo "--- $name ---"
    $CLANG -O0 -Xclang -disable-O0-optnone -S -emit-llvm "$src" -o "$ir"
    $OPT -load-pass-plugin "$SO" -passes="mem2reg,amd-opt" -S "$ir" -o "$out" 2>&1
    echo "  BEFORE: $(grep 'ret i32' $ir)"
    echo "  AFTER:  $(grep 'ret i32' $out)"
    echo ""
done

echo "=== All done. Optimized IR files in build/: ==="
ls -1 build/opt_*.ll
