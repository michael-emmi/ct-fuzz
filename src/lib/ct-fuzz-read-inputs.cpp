#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "ct-fuzz-read-inputs.h"
#include "ct-fuzz-instrument-utils.h"

using namespace llvm;
typedef CTFuzzInstrumentUtils Utils;

unsigned getIndirectionLevel(Type* T) {
  if (!T->isPointerTy())
    return 0;
  else
    return 1 + getIndirectionLevel(cast<PointerType>(T)->getElementType());
}

Type* getUltimateElemTy(Type* T) {
  if (!T->isPointerTy())
    return T;
  else
    return getUltimateElemTy(cast<PointerType>(T)->getElementType());
}

std::string getTypeName(Type* T) {
  if (IntegerType* it = dyn_cast<IntegerType>(T))
    return "i" + std::to_string(it->getBitWidth());
  else if (ArrayType* at = dyn_cast<ArrayType>(T))
    return "arr_" + getTypeName(at->getElementType());
  else if (StructType* st = dyn_cast<StructType>(T))
    // assume named struct types
    return st->getName();
  else if (PointerType* pt = dyn_cast<PointerType>(T))
    return "ptr_" + getTypeName(pt->getElementType());
  else
    llvm_unreachable("doesn't support this type");
}

Function* CTFuzzReadInputs::getReadFunc(Type* elemT, Module* M) {
  PointerType* pt = PointerType::getUnqual(elemT);
  Type* sizeT = Utils::getSecondArg(stdinRF)->getType();
  LLVMContext& C = M->getContext();
  auto DL = M->getDataLayout();

  std::string TN = getTypeName(elemT);
  // already got function corresponding to this type
  if (mappings.count(TN))
    return mappings[TN];
  std::string FN = "__ct_fuzz_read_" + TN;
  FunctionType* FT = FunctionType::get(Type::getVoidTy(C), {pt}, false);
  Function* F = Function::Create(FT, GlobalValue::InternalLinkage, FN, M);
  BasicBlock* B = BasicBlock::Create(C, "", F);
  IRBuilder<> IRB(B);

  if (!elemT->isPointerTy()) {
    // construct non-pointer type read func
    // get the name first
    if (IntegerType* it = dyn_cast<IntegerType>(elemT)) {
      IRB.CreateCall(stdinRF,
        {IRB.CreateBitCast(Utils::getFirstArg(F),
          Utils::getFirstArg(stdinRF)->getType()),
        Utils::getTypeSizeInSizeT(DL, it, sizeT)});
    }
    else if (ArrayType* at = dyn_cast<ArrayType>(elemT)) {
      Type* aet = at->getElementType();
      for (unsigned i = 0; i < at->getNumElements(); ++i)
        IRB.CreateCall(getReadFunc(aet, M),
          {IRB.CreateGEP(Utils::getFirstArg(F),
            {ConstantInt::get(IntegerType::get(C, 64), i)})});
    } else if (StructType* st = dyn_cast<StructType>(elemT)) {
      for (unsigned i = 0; i < st->getNumElements(); ++i) {
        Type* set = st->getElementType(i);
        IRB.CreateCall(getReadFunc(set, M),
          {IRB.CreateGEP(Utils::getFirstArg(F),
            {ConstantInt::get(IntegerType::get(C, 64), 0),
            ConstantInt::get(IntegerType::get(C, 32), i)})});
      }
    } else
      llvm_unreachable("doesn't support this type");
  } else {
    // pointer type, we should leverage generic read pointer function
    Type* uet = getUltimateElemTy(elemT);
    PointerType* pet = cast<PointerType>(elemT);
    unsigned indirectionLevel = getIndirectionLevel(pet->getElementType());
    IRB.CreateCall(genericPtrReadF,
      {IRB.CreateBitCast(Utils::getFirstArg(F), Utils::getFirstArg(genericPtrReadF)->getType()),
      Utils::getTypeSizeInSizeT(DL, uet, sizeT),
      ConstantInt::get(IntegerType::get(C, 32), indirectionLevel),
      IRB.CreateBitCast(getReadFunc(uet, M), Utils::getLastArg(genericPtrReadF)->getType())});
  }
  IRB.CreateRetVoid();
  mappings[TN] = F;
  return F;
}

bool CTFuzzReadInputs::runOnFunction(Function& F) {
  stdinRF = F.getParent()->getFunction("__ct_fuzz_stdin_read");
  genericPtrReadF = F.getParent()->getFunction("__ct_fuzz_read_ptr_generic");
  IRBuilder<> IRB(&*(F.getEntryBlock().getFirstInsertionPt()));
  for (auto& arg: F.args()) {
    if (F.getName() == "f") {
      auto box = IRB.CreateAlloca(arg.getType());
      Function* RF = getReadFunc(arg.getType(), F.getParent());
      IRB.CreateCall(RF, {box});
      arg.replaceAllUsesWith(IRB.CreateLoad(box));
    }
  }
  return false;
}

char CTFuzzReadInputs::ID = 0;

static RegisterPass<CTFuzzReadInputs> X("ct-fuzz-read-inputs", "'deserialization' pass");
