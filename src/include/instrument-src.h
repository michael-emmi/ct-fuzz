#ifndef INSTRUMENT_SRC_H
#define INSTRUMENT_SRC_H

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/InstVisitor.h"

namespace CTFuzz {

using namespace llvm;

class InstrumentSrc: public FunctionPass, public InstVisitor<InstrumentSrc> {
  public:
    static char ID;
    InstrumentSrc() : FunctionPass(ID) {}
    virtual bool runOnFunction(Function& F);
    void visitLoadInst(LoadInst& li);
    void visitStoreInst(StoreInst& si);
    void visitBranchInst(BranchInst& bi);
    void visitSwitchInst(SwitchInst& swi);

  private:
    Function* updateOnCondFunc;
    Function* updateOnAddrFunc;
    void getOrBuildUpdateFuncs(Module* M);
};

}

#endif
