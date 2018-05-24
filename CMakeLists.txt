cmake_minimum_required(VERSION 3.11.2)
project(ct-fuzz)

add_library(afl-instrument STATIC
  afl-2.52b/llvm_mode/afl-llvm-pass.so.cc
)
