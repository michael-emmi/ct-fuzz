#include "options.h"
#include "llvm/Support/CommandLine.h"

namespace CTFuzz {
using namespace llvm;

const cl::opt<std::string> Options::EntryPoint(
  "entry-point",
  cl::desc("Entry point function name")
);

const cl::opt<unsigned> Options::SeedNum(
  "seed-num",
  cl::desc("The number of seeds automatically generated"),
  cl::init(0)
);
}
