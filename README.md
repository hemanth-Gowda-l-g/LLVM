# LLVM Optimizer Pass

This repository contains a custom LLVM function pass plugin (`llvm-opt`) that performs:
- Constant folding via LLVM's `ConstantFoldInstruction`
- Dead code elimination for trivially dead, side-effect-free instructions

## Project layout

- `src/LLVMOptimizerPass.cpp`: LLVM pass implementation
- `examples/*.c`: Small C examples used as optimization test inputs
- `scripts/run_examples.sh`: End-to-end build and test runner
- `build/`: Generated build artifacts (ignored by git)

## Prerequisites

Install toolchain in WSL/Ubuntu:

```bash
sudo apt update
sudo apt install -y cmake clang llvm-dev llvm
```

## Build only

```bash
cmake -S . -B build
cmake --build build
```

## Build and run all examples

```bash
bash scripts/run_examples.sh
```

This script will:
1. Build the pass plugin (`build/libLLVMOptimizer.so`)
2. Compile each file in `examples/` to LLVM IR
3. Run `llvm-opt` on each IR file
4. Produce optimized files as `build/opt_<name>.ll`
