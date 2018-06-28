#ifndef INSTRUMENT_SELF_H
#define INSTRUMENT_SELF_H

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "read-inputs.h"

#include <vector>

using namespace llvm;

typedef unsigned char idx_t;
typedef std::vector<AllocaInst*> BoxList;
typedef iterator_range<Function::arg_iterator> argsT;

namespace CTFuzz {
class InstrumentSelf: public ModulePass {
  public:
    static char ID;
    InstrumentSelf() : ModulePass(ID) {}
    virtual bool runOnModule(Module& M);

  private:
    Function* buildPublicInHandleFunc(CallInst* CI);
    void insertPublicInHandleFuncs(Module& M);
    void readInputs(CallInst* TCI,
      argsT& args, BoxList& boxes, ReadInputs& ri);
    void checkInputs(CallInst* TCI,
      const BoxList& boxes, Function* specF);
    void mergePtrInputs(CallInst* TCI,
      argsT& args,
      const BoxList& boxes, BoxList& ptrBoxes,
      ReadInputs& ri);
    void execInputFunc(CallInst* TCI,
      argsT& args,
      const BoxList& boxes, const BoxList& ptrBoxes,
      Function* srcF, ReadInputs& ri);
    void generateSeeds(Function* F);
    void generateSeedForT(Type* T);
};
}

#endif
