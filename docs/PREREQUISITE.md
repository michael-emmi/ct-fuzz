# External Libraries Required

## jemalloc
Any deterministic memory allocator should work, so we choose jemalloc as the default one.

## xxHash
We use [xxHash](https://github.com/Cyan4973/xxHash)'s hash functions to record observations.

# Tools Required

## american fuzzy lop
Afl is the default fuzzer for ct-fuzz.

## LLVM lit
We use LLVM lit as ct-fuzz's testing infrastructure.



