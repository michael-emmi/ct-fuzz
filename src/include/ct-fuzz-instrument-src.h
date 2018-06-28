#ifndef CT_FUZZ_INSTRUMENT_SRC_H
#define CT_FUZZ_INSTRUMENT_SRC_H

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/InstVisitor.h"

class CTFuzzInstrumentSrc: public llvm::FunctionPass, public llvm::InstVisitor<CTFuzzInstrumentSrc> {
  public:
    static char ID;
    CTFuzzInstrumentSrc() : llvm::FunctionPass(ID) {}
    virtual bool runOnFunction(llvm::Function& F);
    void visitLoadInst(llvm::LoadInst& li);
    void visitStoreInst(llvm::StoreInst& si);
    void visitBranchInst(llvm::BranchInst& bi);
    void visitSwitchInst(llvm::SwitchInst& swi);

  private:
    llvm::Function* updateOnCondFunc;
    llvm::Function* updateOnAddrFunc;
    void getOrBuildUpdateFuncs(llvm::Module* M);
};

#endif
