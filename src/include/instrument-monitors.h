#ifndef INSTRUMENT_MONITORS_H
#define INSTRUMENT_MONITORS_H

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/InstVisitor.h"

namespace CTFuzz {

using namespace llvm;

class InstrumentMonitors: public FunctionPass, public InstVisitor<InstrumentMonitors> {
  public:
    static char ID;
    InstrumentMonitors() : FunctionPass(ID) {}
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
