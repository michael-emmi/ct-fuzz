#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"
#include "instrument-monitors.h"

#include <tuple>

#define UPDATE_BY_COND_FUNC "__ct_fuzz_update_monitor_by_cond"
#define UPDATE_BY_ADDR_FUNC "__ct_fuzz_update_monitor_by_addr"

using namespace llvm;

namespace {
typedef std::tuple<std::string, int, int> DbgInfoT;


ConstantInt* mkChar(unsigned char c, LLVMContext& C) {
  return ConstantInt::get(Type::getInt8Ty(C), c);
}

ConstantInt* mkNum(unsigned n, LLVMContext& C) {
  return ConstantInt::get(Type::getInt32Ty(C), n);
}

ConstantInt* mkZeroIdx(LLVMContext& C) {
  return ConstantInt::get(Type::getInt64Ty(C), 0);
}

std::string getGlobalName(std::string fn) {
  static const std::string GLOBAL_PREFIX = "__ct_fuzz_function_name_";
  return GLOBAL_PREFIX +
    std::to_string(std::hash<std::string>{}(fn));
}

GlobalVariable* mkFileNameStrGlobal(Module* M, std::string fn) {
  LLVMContext& C = M->getContext();
  IntegerType* IT = Type::getInt8Ty(C);
  ArrayType* AT = ArrayType::get(IT, fn.size()+1);

  std::vector<Constant*> vs;
  for (auto c : fn)
    vs.push_back(mkChar(c, C));
  vs.push_back(mkChar(0, C));

  return new GlobalVariable(*M,
    AT, // is an array type
    true, // is a constant
    GlobalVariable::PrivateLinkage,
    ConstantArray::get(AT, vs),
    getGlobalName(fn)
   );
}

GlobalVariable* getFileNameStrGlobal(Module* M, std::string fn) {
  GlobalVariable* fng = M->getGlobalVariable(getGlobalName(fn), true);
  if (!fng)
    fng = mkFileNameStrGlobal(M, fn);
  return fng;
}

// higher optimization levels may erase
// debug location; for this case, we add
// a special debug location with line and column
// numbers both being 0 as well as having
// the function's scope
void setDebugLocOnDemand(Instruction* I) {
  auto P = I->getFunction()->getSubprogram();
  if ((!I->getMetadata("dbg")) && P)
    I->setDebugLoc(DebugLoc::get(0,0,P));
}

DbgInfoT getDbgInfo(const Instruction* I) {
  std::string fn = "";
  unsigned ln = 0;
  unsigned cl = 0;
  if (I->getMetadata("dbg")) {
    const DebugLoc DL = I->getDebugLoc();
    auto *scope = cast<DIScope>(DL.getScope());
    fn = scope->getFilename().str();
    ln = DL.getLine();
    cl = DL.getCol();
  }
  return std::make_tuple(fn, ln, cl);
}

}

namespace CTFuzz {

void InstrumentMonitors::visitLoadInst(LoadInst& li) {
  IRBuilder<> IRB(&li);
  DbgInfoT di = getDbgInfo(&li);
  Module* M = li.getModule();
  LLVMContext& C = M->getContext();

  auto CI = IRB.CreateCall(updateOnAddrFunc,
    {IRB.CreateBitCast(li.getPointerOperand(),
      Type::getInt8PtrTy(C)),
    IRB.CreateGEP(getFileNameStrGlobal(M, std::get<0>(di)),
      {mkZeroIdx(C), mkZeroIdx(C)}),
    mkNum(std::get<1>(di), C),
    mkNum(std::get<2>(di), C)
    });

  setDebugLocOnDemand(CI);
}

void InstrumentMonitors::visitStoreInst(StoreInst& si) {
  IRBuilder<> IRB(&si);
  DbgInfoT di = getDbgInfo(&si);
  Module* M = si.getModule();
  LLVMContext& C = M->getContext();

  auto CI = IRB.CreateCall(updateOnAddrFunc,
    {IRB.CreateBitCast(si.getPointerOperand(),
      Type::getInt8PtrTy(C)),
    IRB.CreateGEP(getFileNameStrGlobal(M, std::get<0>(di)),
      {mkZeroIdx(C), mkZeroIdx(C)}),
    mkNum(std::get<1>(di), C),
    mkNum(std::get<2>(di), C)
    });

  setDebugLocOnDemand(CI);
}

void InstrumentMonitors::visitBranchInst(BranchInst& bi) {
  if (bi.isConditional()) {
    IRBuilder<> IRB(&bi);
    DbgInfoT di = getDbgInfo(&bi);
    Module* M = bi.getModule();
    LLVMContext& C = M->getContext();

    IRB.CreateCall(updateOnCondFunc,
      {bi.getCondition(),
      IRB.CreateGEP(getFileNameStrGlobal(M, std::get<0>(di)),
        {mkZeroIdx(C), mkZeroIdx(C)}),
      mkNum(std::get<1>(di), C),
      mkNum(std::get<2>(di), C)
      });
  }
}

void InstrumentMonitors::visitSwitchInst(SwitchInst& swi) {
  llvm_unreachable("Not really expect to see switchinsts.");
}

// TODO: handle more terminitor instructions

void InstrumentMonitors::getOrBuildUpdateFuncs(Module* M) {
  LLVMContext& C = M->getContext();
  auto getOrBuildUpdateFunc = [&M, &C](Type* argT, std::string name) -> Function* {
    FunctionType* FT = FunctionType::get(Type::getVoidTy(C),
      {argT, Type::getInt8PtrTy(C), Type::getInt32Ty(C), Type::getInt32Ty(C)},
      false) ;
    return cast<Function>(M->getOrInsertFunction(name, FT));
  };
  updateOnCondFunc = getOrBuildUpdateFunc(Type::getInt1Ty(C), UPDATE_BY_COND_FUNC);
  updateOnAddrFunc = getOrBuildUpdateFunc(Type::getInt8PtrTy(C), UPDATE_BY_ADDR_FUNC);
}

bool InstrumentMonitors::runOnFunction(Function& F) {
  getOrBuildUpdateFuncs(F.getParent());

  auto isCTFuzzFunc = [](std::string name) -> bool {
    return name.find("__ct_fuzz") == 0;
  };

  if (!F.hasName() ||
      (!isCTFuzzFunc(F.getName()) && F.getName() != "main"))
    this->visit(F);
  return false;
}

// Pass ID variable
char InstrumentMonitors::ID = 0;
}

static void registerThisPass(const PassManagerBuilder &,
                           llvm::legacy::PassManagerBase &PM) {
    PM.add(createLowerSwitchPass());
    PM.add(new CTFuzz::InstrumentMonitors());
}

static RegisterStandardPasses
    RegisterThisPass_0(PassManagerBuilder::EP_OptimizerLast,
                   registerThisPass);
// we need this because llvm treats custom pass as optimization
// and thus doesn't invoke it on O0 by default
static RegisterStandardPasses
    RegisterThisPass_1(PassManagerBuilder::EP_EnabledOnOptLevel0,
                   registerThisPass);
