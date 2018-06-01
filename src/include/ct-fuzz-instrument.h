#ifndef CT_FUZZ_INSTRUMENT_H
#define CT_FUZZ_INSTRUMENT_H

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"

class CTFuzzInstrument: public llvm::ModulePass {
  public:
    static char ID;
    CTFuzzInstrument() : llvm::ModulePass(ID) {}
    virtual bool runOnModule(llvm::Module& M);
};

#endif
