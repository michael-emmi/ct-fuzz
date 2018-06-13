#ifndef CT_FUZZ_INSTRUMENT_SRC_H
#define CT_FUZZ_INSTRUMENT_SRC_H

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/InstVisitor.h"

class CTFuzzInstrumentSrc: public llvm::ModulePass, public llvm::InstVisitor<CTFuzzInstrumentSrc> {
  public:
    static char ID;
    CTFuzzInstrumentSrc() : llvm::ModulePass(ID) {}
    virtual bool runOnModule(llvm::Module& M);
    void visitLoadInst(llvm::LoadInst& li);
    void visitStoreInst(llvm::StoreInst& si);
    void visitBranchInst(llvm::BranchInst& bi);
    void visitSwitchInst(llvm::SwitchInst& swi);

  private:
    llvm::Function* updateOnCondFunc;
    llvm::Function* updateOnAddrFunc;
};

#endif
