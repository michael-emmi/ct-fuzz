#ifndef CT_FUZZ_OPTIONS
#define CT_FUZZ_OPTIONS

#include "llvm/Support/CommandLine.h"

class CTFuzzOptions {
  public:
    static const llvm::cl::opt<std::string> EntryPoint;
    static const llvm::cl::opt<unsigned> SeedNum;
};
#endif
