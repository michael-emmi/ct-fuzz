# CT-Fuzz

The fuzzer for constant time.

## Requirements

* The [Python] platform, version 2.7
* The [LLVM] compiler infrastructure, version >= 5.0
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

# run tests (optional)
cd ../test && lit .
```

## More information

See [usage].

[Python]: https://www.python.org
[LLVM]: http://llvm.org
[jemalloc]: http://jemalloc.net
[usage]: docs/USAGE.md
