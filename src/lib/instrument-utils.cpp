#include "llvm/IR/InstIterator.h"
#include "instrument-utils.h"
#include <vector>
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace CTFuzz {

Function* getFunction(Module& M, std::string name) {
  Function* f = M.getFunction(name);
  assert(f && "Unable to get function in this module.");
  return f;
}

Value* getSecondArg(Function* F) {
  auto I = F->arg_begin();
  return &*(++I);
}

Value* getFirstArg(Function* F) {
  return &*(F->arg_begin());
}

Value* getLastArg(Function* F) {
  auto I = F->arg_end();
  return &*(--I);
}

std::vector<CallInst*> getCallToFunc(Function* F) {
  std::vector<CallInst*> cis;
  if (!F)
    return cis;
  std::function<void(Value*)> addCallToF = [&cis, &addCallToF](Value* V) -> void {
    for (auto U: V->users()) {
      if (CallInst* ci = dyn_cast<CallInst>(U))
        cis.push_back(ci);
      else if (isa<BitCastInst>(U))
        addCallToF(U);
      else if (auto ce = dyn_cast<ConstantExpr>(U))
        if (isa<BitCastInst>(ce->getAsInstruction()))
          addCallToF(U);
    }
  };
  addCallToF(F);
  return cis;
}

CallInst* getCallToFuncOnce(Function* F) {
  auto ret = getCallToFunc(F);
  assert(ret.size() == 1 && "Should see just one call site.");
  return ret[0];
}

IntegerType* getLenT(LLVMContext& C) {
  return IntegerType::get(C, 16);
}

Constant* getTypeSizeInSizeT(DataLayout& DL, Type* T, Type* sizeT) {
  return ConstantInt::get(sizeT, DL.getTypeStoreSize(T));
}

Value* getByteSizeInSizeT(IRBuilder<>& IRB, DataLayout& DL, Value* len, Type* elemT, Type* sizeT) {
  assert(!elemT->isVoidTy() && "what's the size of void type?");
  return IRB.CreateMul(IRB.CreateZExt(len, sizeT), getTypeSizeInSizeT(DL, elemT, sizeT));
}
}
