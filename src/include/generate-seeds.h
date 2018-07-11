#ifndef GENERATE_SEEDS
#define GENERATE_SEEDS
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"

#include <sstream>

using namespace llvm;

namespace CTFuzz {
class GenerateSeeds: public FunctionPass {
  public:
    static char ID;
    GenerateSeeds() : FunctionPass(ID) {}
    virtual bool runOnFunction(Function& F);

  private:
    void generateSeeds(Function* F, std::ostream& ss);
    void generateSeedForT(Type* T, std::ostream& ss);
};
}

#endif
