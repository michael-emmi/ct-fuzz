#ifndef CT_FUZZ_INSTRUMENT_H
#define CT_FUZZ_INSTRUMENT_H

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/InstVisitor.h"

class CTFuzzInstrument: public llvm::ModulePass, public llvm::InstVisitor<CTFuzzInstrument> {
  public:
    static char ID;
    CTFuzzInstrument() : llvm::ModulePass(ID) {}
    virtual bool runOnModule(llvm::Module& M);
    void visitLoadInst(llvm::LoadInst& li);
    void visitStoreInst(llvm::StoreInst& si);
    void visitBranchInst(llvm::BranchInst& bi);
    void visitSwitchInst(llvm::SwitchInst& swi);

  private:
    llvm::Function* updateOnCondFunc;
    llvm::Function* updateOnAddrFunc;
    void updateMonitors(llvm::Module& M);
};

#endif
