#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <fstream>
#include "generate-seeds.h"

namespace {
void printByte(unsigned char b, std::ostream& ss) {
  char tmp[4];
  sprintf(tmp, "\\x%02X", b);
  ss << tmp;
}

void printInt(uint64_t v, unsigned size, std::ostream& ss) {
  unsigned char* p = (unsigned char*)(&v);
  for (unsigned i = 0; i < size; ++i)
    printByte(p[i], ss);
}

bool flipCoin() {
  return std::rand() & 1U;
}

unsigned short generateLen(Type* ET) {
  if (ET->isStructTy() || ET->isPointerTy())
    return flipCoin()? 1 : 2;
  else {
    bool b1 = flipCoin();
    bool b2 = flipCoin();
    if (b1)
      if (b2)
        return 1;
      else
        return 2;
    else
      if (b2)
        return 3;
      else
        return 4;
  }
}
}

namespace CTFuzz {
const cl::opt<std::string> EntryPoint(
  "entry-point",
  cl::desc("Entry point function name")
);

const cl::opt<unsigned> SeedNum(
  "seed-num",
  cl::desc("The number of seeds automatically generated"),
  cl::init(0)
);

const cl::opt<std::string> OutputFile(
  "output-file",
  cl::desc("The output file that contains the seeds")
);

void GenerateSeeds::generateSeedForT(Type* T, std::ostream& ss) {
  if (!T->isPointerTy())
    if (IntegerType* it = dyn_cast<IntegerType>(T))
      printInt(42, it->getBitWidth() >> 3, ss);
    else if (ArrayType* at = dyn_cast<ArrayType>(T))
      for (unsigned i = 0 ; i < at->getNumElements(); ++i)
        generateSeedForT(at->getElementType(), ss);
    else if (StructType* st = dyn_cast<StructType>(T))
      for (unsigned i = 0 ; i < st->getNumElements(); ++i)
        generateSeedForT(st->getElementType(i), ss);
    else
      llvm_unreachable("doesn't support this type");
  else {
    PointerType* pt = cast<PointerType>(T);
    Type* et = pt->getElementType();
    unsigned short len = generateLen(et);
    printInt(len, 2, ss);
    for (unsigned short i = 0; i < len; ++i)
      generateSeedForT(et, ss);
  }
}

void GenerateSeeds::generateSeeds(Function* F, std::ostream& ss) {
  for (auto& arg: F->args())
    generateSeedForT(arg.getType(), ss);
}

bool GenerateSeeds::runOnFunction(Function& F) {
  if (F.hasName() && F.getName().str() == EntryPoint) {
    std::ofstream f;
    if (OutputFile.getNumOccurrences())
      f.open(OutputFile);

    std::ostream& ss = OutputFile.getNumOccurrences()? f : std::cout;

    if (SeedNum) {
      std::srand(time(NULL));
      for (unsigned i = 0; i < SeedNum; ++i) {
        auto s = std::rand();
        std::srand(s);
        generateSeeds(&F, ss);
        std::srand(s);
        generateSeeds(&F, ss);
        ss << std::endl;
      }
    }

    if (OutputFile.getNumOccurrences())
      static_cast<std::ofstream*>(&ss)->close();
  }

  return true;
}

// Pass ID variable
char GenerateSeeds::ID = 0;

// Register the pass
static RegisterPass<GenerateSeeds>
X("ct-fuzz-generate-seeds", "Seed generation for constant time fuzzer");

}
