#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "instrument-src.h"
#include "naming.h"

using namespace llvm;

namespace CTFuzz {

void InstrumentSrc::visitLoadInst(LoadInst& li) {
  IRBuilder<> IRB(&li);
  auto CI = IRB.CreateCall(updateOnAddrFunc,
    {IRB.CreateBitCast(li.getPointerOperand(), Type::getInt8PtrTy(li.getContext()))});
  CI->setDebugLoc(li.getParent()->begin()->getDebugLoc());
}

void InstrumentSrc::visitStoreInst(StoreInst& si) {
  IRBuilder<> IRB(&si);
  auto CI = IRB.CreateCall(updateOnAddrFunc,
    {IRB.CreateBitCast(si.getPointerOperand(), Type::getInt8PtrTy(si.getContext()))});
  CI->setDebugLoc(si.getParent()->begin()->getDebugLoc());
}

void InstrumentSrc::visitBranchInst(BranchInst& bi) {
  if (bi.isConditional()) {
    IRBuilder<> IRB(&bi);
    IRB.CreateCall(updateOnCondFunc, {bi.getCondition()});
  }
}

void InstrumentSrc::visitSwitchInst(SwitchInst& swi) {
  llvm_unreachable("Not really expect to see switchinsts.");
}

// TODO: handle more terminitor instructions

void InstrumentSrc::getOrBuildUpdateFuncs(Module* M) {
  LLVMContext& C = M->getContext();
  auto getOrBuildUpdateFunc = [&M, &C](Type* argT, std::string name) -> Function* {
    FunctionType* FT = FunctionType::get(Type::getVoidTy(C), {argT}, false) ;
    return cast<Function>(M->getOrInsertFunction(name, FT));
  };
  updateOnCondFunc = getOrBuildUpdateFunc(Type::getInt1Ty(C), "__ct_fuzz_update_monitor_by_cond");
  updateOnAddrFunc = getOrBuildUpdateFunc(Type::getInt8PtrTy(C), "__ct_fuzz_update_monitor_by_addr");
}

bool InstrumentSrc::runOnFunction(Function& F) {
  getOrBuildUpdateFuncs(F.getParent());
  if (!F.hasName() ||
      (!Naming::isCTFuzzFunc(F.getName()) && F.getName() != "main"))
    this->visit(F);
  return false;
}

// Pass ID variable
char InstrumentSrc::ID = 0;
}

static void registerThisPass(const PassManagerBuilder &,
                           llvm::legacy::PassManagerBase &PM) {
    PM.add(new CTFuzz::InstrumentSrc());
}

static RegisterStandardPasses
    RegisterThisPass_0(PassManagerBuilder::EP_OptimizerLast,
                   registerThisPass);
// we need this because llvm treats custom pass as optimization
// and thus doesn't invoke it on O0 by default
static RegisterStandardPasses
    RegisterThisPass_1(PassManagerBuilder::EP_EnabledOnOptLevel0,
                   registerThisPass);
