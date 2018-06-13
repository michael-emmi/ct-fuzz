#ifndef CT_FUZZ_INSTRUMENT_UTILS
#define CT_FUZZ_INSTRUMENT_UTILS
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"

class CTFuzzInstrumentUtils  {
  public:
    static llvm::Function* getFunction(llvm::Module& M, std::string name);
    static llvm::Value* getSecondArg(llvm::Function* F);
    static llvm::Value* getFirstArg(llvm::Function* F);
    static std::vector<llvm::CallInst*> getCallFromFunc(llvm::Function* F, std::string FN);
    static llvm::CallInst* getCallToFuncOnce(llvm::Function* F, std::string FN);
};

#endif
