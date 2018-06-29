#ifndef UTILS
#define UTILS
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Constants.h"

namespace CTFuzz {

using namespace llvm;

Function* getFunction(Module& M, std::string name);
Value* getSecondArg(Function* F);
Value* getFirstArg(Function* F);
Value* getLastArg(Function* F);
std::vector<CallInst*> getCallToFunc(Function* F);
CallInst* getCallToFuncOnce(Function* F);
IntegerType* getLenT(LLVMContext& C);
Constant* getTypeSizeInSizeT(DataLayout& DL, Type* T, Type* sizeT);
Value* getByteSizeInSizeT(IRBuilder<>& IRB, DataLayout& DL, Value* len, Type* elemT, Type* sizeT);

}

#endif
