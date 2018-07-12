#ifndef GENERATE_SEEDS
#define GENERATE_SEEDS
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Instructions.h"

#include <sstream>

using namespace llvm;

namespace CTFuzz {
class GenerateSeeds: public FunctionPass {
  public:
    static char ID;
    GenerateSeeds() : FunctionPass(ID) {}
    virtual bool runOnFunction(Function& F);

  private:
    void generateSeedForInvocation(CallInst* CI, std::ostream& ss);
    void generateSeedForVT(Value* V, Type* T, std::ostream& ss);
};
}

#endif
