# CT-Fuzz

The fuzzer for constant time.

## Requirements

* The [LLVM] compiler infrastructure, version 3.9
* The [jemalloc] memory allocator

## Installation

```bash
# fetch the submodules
git submodule init
git submodule update

# create isolated build directory
mkdir build && cd build

# build libraries and executables
cmake .. && make

# install libraries and executables
make install
```

## More information

See [usage].


[LLVM]: http://llvm.org
[jemalloc]: http://jemalloc.net
[usage]: docs/USAGE.md
