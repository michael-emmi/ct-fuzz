#ifndef CT_FUZZ_INSTRUMENT_SELF_H
#define CT_FUZZ_INSTRUMENT_SELF_H

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/InstVisitor.h"

class CTFuzzInstrumentSelf: public llvm::ModulePass {
  public:
    static char ID;
    CTFuzzInstrumentSelf() : llvm::ModulePass(ID) {}
    virtual bool runOnModule(llvm::Module& M);

  private:
    void updateMonitors(llvm::Module& M);
};

#endif
