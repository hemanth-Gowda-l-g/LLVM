cmake_minimum_required(VERSION 3.10)
project(AMDOptimizer)

find_package(LLVM REQUIRED CONFIG)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

add_library(AMDOptimizer MODULE src/AMDOptimizerPass.cpp)

# Ensure the library is built with the correct flags for LLVM
set_target_properties(AMDOptimizer PROPERTIES COMPILE_FLAGS "-fno-rtti")