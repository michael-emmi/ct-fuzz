#ifndef INSTRUMENT_SELF_H
#define INSTRUMENT_SELF_H

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "naming.h"
#include "read-inputs.h"

#include <vector>

using namespace llvm;

typedef unsigned char idx_t;
typedef std::vector<AllocaInst*> BoxList;

namespace CTFuzz {
class InstrumentSelf: public ModulePass {
  public:
    static char ID;
    InstrumentSelf() : ModulePass(ID) {}
    virtual bool runOnModule(Module& M);

  private:
    Module* M;
    Function* mallocF;
    Function* buildPublicInHandleFunc(CallInst* CI);
    void insertPublicInHandleFuncs(Function* specF);
    void readInputs(CallInst* TCI, iterator_range<Function::arg_iterator>& args, BoxList& boxes);
    void checkInputs(CallInst* TCI, iterator_range<Function::arg_iterator>& args, const BoxList& boxes, Function* specF);
    void mergePtrInputs(CallInst* TCI, iterator_range<Function::arg_iterator>& args, const BoxList& boxes, BoxList& ptrBoxes);
    void execInputFunc(CallInst* TCI, iterator_range<Function::arg_iterator>& args, const BoxList& boxes, const BoxList& ptrBoxes, Function* srcF);
    void generateSeeds(Function* F);
    void generateSeedForT(Type* T);
    ReadInputs* ri;
};
}

#endif
