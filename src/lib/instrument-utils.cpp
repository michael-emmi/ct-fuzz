#include "llvm/IR/InstIterator.h"
#include "instrument-utils.h"
#include <vector>

using namespace llvm;

namespace CTFuzz {

Function* InstrumentUtils::getFunction(Module& M, std::string name) {
  Function* f = M.getFunction(name);
  assert(f && "Unable to get function in this module.");
  return f;
}

Value* InstrumentUtils::getSecondArg(Function* F) {
  auto I = F->arg_begin();
  return &*(++I);
}

Value* InstrumentUtils::getFirstArg(Function* F) {
  return &*(F->arg_begin());
}

Value* InstrumentUtils::getLastArg(Function* F) {
  auto I = F->arg_end();
  return &*(--I);
}

std::vector<CallInst*> InstrumentUtils::getCallFromFunc(Function* F, std::string FN) {
  std::vector<CallInst*> cis;
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    if (CallInst* ci = dyn_cast<CallInst>(&*I)) {
      Function* f = ci->getCalledFunction();
      if (!f)
        f = dyn_cast<Function>(ci->getCalledValue()->stripPointerCasts());
      if (f && f->hasName() && f->getName().str() == FN)
          cis.push_back(ci);
    }
  }
  return cis;
}

CallInst* InstrumentUtils::getCallToFuncOnce(Function* F, std::string FN) {
  auto ret = getCallFromFunc(F, FN);
  assert(ret.size() == 1 && "Should see just one call site.");
  return ret[0];
}

IntegerType* InstrumentUtils::getLenT(LLVMContext& C) {
  return IntegerType::get(C, 16);
}

Constant* InstrumentUtils::getTypeSizeInSizeT(DataLayout& DL, Type* T, Type* sizeT) {
  return ConstantInt::get(sizeT, DL.getTypeStoreSize(T));
}

Value* InstrumentUtils::getByteSizeInSizeT(IRBuilder<>& IRB, DataLayout& DL, Value* len, Type* elemT, Type* sizeT) {
  assert(!elemT->isVoidTy() && "what's the size of void type?");
  return IRB.CreateMul(IRB.CreateZExt(len, sizeT), getTypeSizeInSizeT(DL, elemT, sizeT));
}
}
