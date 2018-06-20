#include "ct-fuzz-instrument-utils.h"
#include <vector>

using namespace llvm;

Function* CTFuzzInstrumentUtils::getFunction(Module& M, std::string name) {
  Function* f = M.getFunction(name);
  assert(f && "Unable to get function in this module.");
  return f;
}

Value* CTFuzzInstrumentUtils::getSecondArg(Function* F) {
  auto I = F->arg_begin();
  return &*(++I);
}

Value* CTFuzzInstrumentUtils::getFirstArg(Function* F) {
  return &*(F->arg_begin());
}

Value* CTFuzzInstrumentUtils::getLastArg(Function* F) {
  auto I = F->arg_end();
  return &*(--I);
}

std::vector<CallInst*> CTFuzzInstrumentUtils::getCallFromFunc(Function* F, std::string FN) {
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

CallInst* CTFuzzInstrumentUtils::getCallToFuncOnce(Function* F, std::string FN) {
  auto ret = getCallFromFunc(F, FN);
  assert(ret.size() == 1 && "Should see just one call site.");
  return ret[0];
}

IntegerType* CTFuzzInstrumentUtils::getLenT(LLVMContext& C) {
  return IntegerType::get(C, 16);
}

Constant* CTFuzzInstrumentUtils::getTypeSizeInSizeT(DataLayout& DL, Type* T, Type* sizeT) {
  return ConstantInt::get(sizeT, DL.getTypeStoreSize(T));
}

Value* CTFuzzInstrumentUtils::getByteSizeInSizeT(IRBuilder<>& IRB, DataLayout& DL, Value* len, Type* elemT, Type* sizeT) {
  assert(!elemT->isVoidTy() && "what's the size of void type?");
  return IRB.CreateMul(IRB.CreateZExt(len, sizeT), getTypeSizeInSizeT(DL, elemT, sizeT));
}
