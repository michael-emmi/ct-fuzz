#ifndef OPTIONS
#define OPTIONS

#include "llvm/Support/CommandLine.h"

using namespace llvm;

namespace CTFuzz {

class Options {
  public:
    static const cl::opt<std::string> EntryPoint;
    static const cl::opt<unsigned> SeedNum;
};
}
#endif
