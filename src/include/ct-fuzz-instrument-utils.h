#ifndef CT_FUZZ_INSTRUMENT_UTILS
#define CT_FUZZ_INSTRUMENT_UTILS
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Constants.h"

class CTFuzzInstrumentUtils  {
  public:
    static llvm::Function* getFunction(llvm::Module& M, std::string name);
    static llvm::Value* getSecondArg(llvm::Function* F);
    static llvm::Value* getFirstArg(llvm::Function* F);
    static llvm::Value* getLastArg(llvm::Function* F);
    static std::vector<llvm::CallInst*> getCallFromFunc(llvm::Function* F, std::string FN);
    static llvm::CallInst* getCallToFuncOnce(llvm::Function* F, std::string FN);
    static llvm::IntegerType* getLenT(llvm::LLVMContext& C);
    static llvm::Constant* getTypeSizeInSizeT(llvm::DataLayout& DL, llvm::Type* T, llvm::Type* sizeT);
    static llvm::Value* getByteSizeInSizeT(llvm::IRBuilder<>& IRB, llvm::DataLayout& DL, llvm::Value* len, llvm::Type* elemT, llvm::Type* sizeT);
};

#endif
