#ifndef INSTRUMENT_UTILS
#define INSTRUMENT_UTILS
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Constants.h"

namespace CTFuzz {

using namespace llvm;
class InstrumentUtils  {
  public:
    static Function* getFunction(Module& M, std::string name);
    static Value* getSecondArg(Function* F);
    static Value* getFirstArg(Function* F);
    static Value* getLastArg(Function* F);
    static std::vector<CallInst*> getCallFromFunc(Function* F, std::string FN);
    static CallInst* getCallToFuncOnce(Function* F, std::string FN);
    static IntegerType* getLenT(LLVMContext& C);
    static Constant* getTypeSizeInSizeT(DataLayout& DL, Type* T, Type* sizeT);
    static Value* getByteSizeInSizeT(IRBuilder<>& IRB, DataLayout& DL, Value* len, Type* elemT, Type* sizeT);
};
}

#endif
