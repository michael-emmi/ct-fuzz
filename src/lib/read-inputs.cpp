#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "read-inputs.h"
#include "utils.h"

#define STDIN_READ_FUNC "__ct_fuzz_stdin_read"
#define READ_PTR_GENERIC "__ct_fuzz_read_ptr_generic"
#define MERGE_PTR_GENERIC "__ct_fuzz_merge_ptr_generic"
#define COPY_PTR_GENERIC "__ct_fuzz_deep_copy_ptr_generic"
#define MEMCPY_WRAPPER "__ct_fuzz_memcpy_wrapper"

using namespace llvm;

namespace CTFuzz {
ReadInputs::ReadInputs(Module* M) : M(M) {
  stdinRF = getFunction(*M, STDIN_READ_FUNC);
  genericPtrReadF = getFunction(*M, READ_PTR_GENERIC);
  genericPtrMergeF = getFunction(*M, MERGE_PTR_GENERIC);
  genericPtrCopyF = getFunction(*M, COPY_PTR_GENERIC);
  memcpyF = getFunction(*M, MEMCPY_WRAPPER);
    }

unsigned ReadInputs::getIndirectionLevel(Type* T) {
  if (!T->isPointerTy())
    return 0;
  else
    return 1 + getIndirectionLevel(cast<PointerType>(T)->getElementType());
}

Type* ReadInputs::getUltimateElemTy(Type* T) {
  if (!T->isPointerTy())
    return T;
  else
    return getUltimateElemTy(cast<PointerType>(T)->getElementType());
}

std::string ReadInputs::getTypeName(Type* T) {
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

Function* ReadInputs::getReadFunc(Type* elemT) {
  PointerType* pt = PointerType::getUnqual(elemT);
  Type* sizeT = getSecondArg(stdinRF)->getType();
  LLVMContext& C = M->getContext();
  auto DL = M->getDataLayout();

  std::string TN = getTypeName(elemT);
  // already got function corresponding to this type
  if (readf_mappings.count(TN))
    return readf_mappings[TN];
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
        {IRB.CreateBitCast(getFirstArg(F),
          getFirstArg(stdinRF)->getType()),
        getTypeSizeInSizeT(DL, it, sizeT)});
    }
    else if (ArrayType* at = dyn_cast<ArrayType>(elemT)) {
      Type* aet = at->getElementType();
      for (unsigned i = 0; i < at->getNumElements(); ++i)
        IRB.CreateCall(getReadFunc(aet),
          {IRB.CreateGEP(getFirstArg(F),
            {ConstantInt::get(IntegerType::get(C, 64), 0),
            ConstantInt::get(IntegerType::get(C, 64), i)})});
    } else if (StructType* st = dyn_cast<StructType>(elemT)) {
      for (unsigned i = 0; i < st->getNumElements(); ++i) {
        Type* set = st->getElementType(i);
        IRB.CreateCall(getReadFunc(set),
          {IRB.CreateGEP(getFirstArg(F),
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
      {IRB.CreateBitCast(getFirstArg(F), getFirstArg(genericPtrReadF)->getType()),
      getTypeSizeInSizeT(DL, uet, sizeT),
      ConstantInt::get(IntegerType::get(C, 32), indirectionLevel),
      IRB.CreateBitCast(getReadFunc(uet), getLastArg(genericPtrReadF)->getType())});
  }

  IRB.CreateRetVoid();
  readf_mappings[TN] = F;
  return F;
}

Function* ReadInputs::getMergeFunc(Type* elemT) {
  Type* sizeT = getSecondArg(stdinRF)->getType();
  PointerType* pt = PointerType::getUnqual(elemT);
  LLVMContext& C = M->getContext();
  auto DL = M->getDataLayout();

  std::string TN = getTypeName(elemT);
  // already got function corresponding to this type
  if (mergef_mappings.count(TN))
    return mergef_mappings[TN];
  std::string FN = "__ct_fuzz_merge_" + TN;
  FunctionType* FT = FunctionType::get(Type::getVoidTy(C), {pt, pt, pt}, false);
  Function* F = Function::Create(FT, GlobalValue::InternalLinkage, FN, M);
  BasicBlock* B = BasicBlock::Create(C, "", F);
  IRBuilder<> IRB(B);

  if (!elemT->isPointerTy()) {
    if (IntegerType* it = dyn_cast<IntegerType>(elemT)) {
      // pass
    } else if (ArrayType* at = dyn_cast<ArrayType>(elemT)) {
      Type* aet = at->getElementType();
      for (unsigned i = 0; i < at->getNumElements(); ++i) {
        auto idx_1 = ConstantInt::get(IntegerType::get(C, 64), 0);
        auto idx_2 = ConstantInt::get(IntegerType::get(C, 64), i);
        if (aet->isPointerTy())
          IRB.CreateCall(getMergeFunc(aet),
              {IRB.CreateGEP(getFirstArg(F), {idx_1, idx_2}),
              IRB.CreateGEP(getSecondArg(F), {idx_1, idx_2}),
              IRB.CreateGEP(getLastArg(F), {idx_1, idx_2})});
      }
    } else if (StructType* st = dyn_cast<StructType>(elemT)) {
      for (unsigned i = 0; i < st->getNumElements(); ++i) {
        auto idx_1 = ConstantInt::get(IntegerType::get(C, 64), 0);
        auto idx_2 = ConstantInt::get(IntegerType::get(C, 32), i);
        Type* set = st->getElementType(i);
        if (set->isPointerTy())
          IRB.CreateCall(getMergeFunc(set),
            {IRB.CreateGEP(getFirstArg(F), {idx_1, idx_2}),
            IRB.CreateGEP(getSecondArg(F), {idx_1, idx_2}),
            IRB.CreateGEP(getLastArg(F), {idx_1, idx_2})});
      }
    } else
      llvm_unreachable("doesn't support this type");
  } else {
    // pointer type, we should leverage generic read pointer function
    Type* uet = getUltimateElemTy(elemT);
    PointerType* pet = cast<PointerType>(elemT);
    unsigned indirectionLevel = getIndirectionLevel(pet->getElementType());
    IRB.CreateCall(genericPtrMergeF,
      {IRB.CreateBitCast(getFirstArg(F), getFirstArg(genericPtrMergeF)->getType()),
      IRB.CreateBitCast(getSecondArg(F), getFirstArg(genericPtrMergeF)->getType()),
      IRB.CreateBitCast(getLastArg(F), getFirstArg(genericPtrMergeF)->getType()),
      getTypeSizeInSizeT(DL, uet, sizeT),
      ConstantInt::get(IntegerType::get(C, 32), indirectionLevel),
      IRB.CreateBitCast(getMergeFunc(uet), getLastArg(genericPtrMergeF)->getType())});
  }

  IRB.CreateRetVoid();
  mergef_mappings[TN] = F;
  return F;
}

Function* ReadInputs::getCopyFunc(Type* elemT) {
  PointerType* pt = PointerType::getUnqual(elemT);
  Type* sizeT = getSecondArg(stdinRF)->getType();
  LLVMContext& C = M->getContext();
  auto DL = M->getDataLayout();

  std::string TN = getTypeName(elemT);
  // already got function corresponding to this type
  if (copyf_mappings.count(TN))
    return copyf_mappings[TN];
  std::string FN = "__ct_fuzz_copy_" + TN;
  FunctionType* FT = FunctionType::get(Type::getVoidTy(C), {pt, pt}, false);
  Function* F = Function::Create(FT, GlobalValue::InternalLinkage, FN, M);
  BasicBlock* B = BasicBlock::Create(C, "", F);
  IRBuilder<> IRB(B);

  if (!elemT->isPointerTy()) {
    if (IntegerType* it = dyn_cast<IntegerType>(elemT)) {
      IRB.CreateCall(memcpyF,
        {IRB.CreateBitCast(getFirstArg(F),
          getFirstArg(memcpyF)->getType()),
        IRB.CreateBitCast(getSecondArg(F),
          getSecondArg(memcpyF)->getType()),
        getTypeSizeInSizeT(DL, it, sizeT)});
    }
    else if (ArrayType* at = dyn_cast<ArrayType>(elemT)) {
      Type* aet = at->getElementType();
      for (unsigned i = 0; i < at->getNumElements(); ++i) {
        auto idx_1 = ConstantInt::get(IntegerType::get(C, 64), 0);
        auto idx_2 = ConstantInt::get(IntegerType::get(C, 64), i);
        IRB.CreateCall(getCopyFunc(aet),
          {IRB.CreateGEP(getFirstArg(F), {idx_1, idx_2}),
          IRB.CreateGEP(getSecondArg(F), {idx_1, idx_2})});
      }
    } else if (StructType* st = dyn_cast<StructType>(elemT)) {
      for (unsigned i = 0; i < st->getNumElements(); ++i) {
        Type* set = st->getElementType(i);
        auto idx_1 = ConstantInt::get(IntegerType::get(C, 64), 0);
        auto idx_2 = ConstantInt::get(IntegerType::get(C, 32), i);
        IRB.CreateCall(getCopyFunc(set),
          {IRB.CreateGEP(getFirstArg(F), {idx_1, idx_2}),
          IRB.CreateGEP(getSecondArg(F), {idx_1, idx_2})});
      }
    } else
      llvm_unreachable("doesn't support this type");
  } else {
    // pointer type, we should leverage generic read pointer function
    Type* uet = getUltimateElemTy(elemT);
    PointerType* pet = cast<PointerType>(elemT);
    unsigned indirectionLevel = getIndirectionLevel(pet->getElementType());
    IRB.CreateCall(genericPtrCopyF,
      {IRB.CreateBitCast(getFirstArg(F), getFirstArg(genericPtrCopyF)->getType()),
      IRB.CreateBitCast(getSecondArg(F), getSecondArg(genericPtrCopyF)->getType()),
      getTypeSizeInSizeT(DL, uet, sizeT),
      ConstantInt::get(IntegerType::get(C, 32), indirectionLevel),
      IRB.CreateBitCast(getCopyFunc(uet), getLastArg(genericPtrCopyF)->getType())});
  }

  IRB.CreateRetVoid();
  copyf_mappings[TN] = F;
  return F;
}
}
