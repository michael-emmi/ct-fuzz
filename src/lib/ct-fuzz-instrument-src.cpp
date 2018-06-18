#include "ct-fuzz-instrument-src.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/IRBuilder.h"
#include "ct-fuzz-instrument-utils.h"
#include "ct-fuzz-naming.h"

#include <utility>
#include <vector>
#include <map>

using namespace llvm;

void CTFuzzInstrumentSrc::visitLoadInst(LoadInst& li) {
  IRBuilder<> IRB(&li);
  auto CI = IRB.CreateCall(updateOnAddrFunc,
    {IRB.CreateBitCast(li.getPointerOperand(), Type::getInt8PtrTy(li.getContext()))});
  CI->setDebugLoc(li.getParent()->begin()->getDebugLoc());
}

void CTFuzzInstrumentSrc::visitStoreInst(StoreInst& si) {
  IRBuilder<> IRB(&si);
  auto CI = IRB.CreateCall(updateOnAddrFunc,
    {IRB.CreateBitCast(si.getPointerOperand(), Type::getInt8PtrTy(si.getContext()))});
  CI->setDebugLoc(si.getParent()->begin()->getDebugLoc());
}

void CTFuzzInstrumentSrc::visitBranchInst(BranchInst& bi) {
  if (bi.isConditional()) {
    IRBuilder<> IRB(&bi);
    IRB.CreateCall(updateOnCondFunc, {bi.getCondition()});
  }
}

void CTFuzzInstrumentSrc::visitSwitchInst(SwitchInst& swi) {
  llvm_unreachable("Not really expect to see switchinsts.");
}

bool CTFuzzInstrumentSrc::runOnModule(Module& M) {
  updateOnCondFunc = CTFuzzInstrumentUtils::getFunction(M, "__ct_fuzz_update_monitor_by_cond");
  updateOnAddrFunc = CTFuzzInstrumentUtils::getFunction(M, "__ct_fuzz_update_monitor_by_addr");	
  for (auto& F : M.functions())
    if (!F.hasName() ||
          (!Naming::isCTFuzzFunc(F.getName()) && F.getName() != "main"))
      this->visit(F);
  return false;
}

// Pass ID variable
char CTFuzzInstrumentSrc::ID = 0;

// Register the pass
//static RegisterPass<CTFuzzInstrumentSrc>
//X("ct-fuzz-instrument-src", "Instrumentations of the input code for constant time fuzzer");

static void registerThisPass(const PassManagerBuilder &,
                           llvm::legacy::PassManagerBase &PM) {
    PM.add(new CTFuzzInstrumentSrc());
}

static RegisterStandardPasses
    //RegisterMyPass(PassManagerBuilder::EP_OptimizerLast,
    RegisterMyPass(PassManagerBuilder::EP_EnabledOnOptLevel0,
                   registerThisPass);
