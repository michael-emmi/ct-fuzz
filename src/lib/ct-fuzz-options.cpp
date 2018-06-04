#include "ct-fuzz-options.h"
#include "llvm/Support/CommandLine.h"

const llvm::cl::opt<std::string> CTFuzzOptions::EntryPoint(
    "entry-point",
    llvm::cl::desc("Entry point function name")
);
