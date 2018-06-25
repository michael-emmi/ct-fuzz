#include "ct-fuzz-options.h"
#include "llvm/Support/CommandLine.h"

const llvm::cl::opt<std::string> CTFuzzOptions::EntryPoint(
  "entry-point",
  llvm::cl::desc("Entry point function name")
);

const llvm::cl::opt<unsigned> CTFuzzOptions::SeedNum(
  "seed-num",
  llvm::cl::desc("The number of seeds automatically generated"),
  llvm::cl::init(0)
);
