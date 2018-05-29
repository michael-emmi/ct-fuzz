#ifndef CT_FUZZ_EXTRACT_EXPR_H
#define CT_FUZZ_EXTRACT_EXPR_H

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/LoopInfo.h"

using namespace llvm;

class ExtractExprs : public ModulePass {
  public:
    static char ID;
    ExtractExprs() : ModulePass(ID) {}
    virtual bool runOnModule(Module& M);
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
};

#endif
