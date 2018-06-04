#include "ct-fuzz-instrument.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DerivedTypes.h"
#include "ct-fuzz-naming.h"
#include "ct-fuzz-options.h"

#include <utility>
#include <vector>
#include <map>

using namespace llvm;

typedef std::pair<Value*, Value*> argInfo;
typedef std::pair<argInfo, argInfo> argInfoP;

Function* getFunction(Module& M, std::string name) {
  Function* f = M.getFunction(name);
  assert(f && "Unable to get function in this module.");
  return f;
}

Function* buildPublicInHandleFunc(Module* M, Value* V) {
  static unsigned counter = 0;
  Type* T = V->getType();
  auto DL = M->getDataLayout();

  Function* CF = getFunction(*M, Naming::PUBLIC_IN_HANDLE_FUNC);

  FunctionType* newFT = FunctionType::get(Type::getVoidTy(M->getContext()), {V->getType()}, false);
  std::string newFN = Naming::PUBLIC_IN_FUNC + "_" + std::to_string(counter);
  Function* newF = Function::Create(newFT, GlobalValue::InternalLinkage, newFN, M);

  BasicBlock* B = BasicBlock::Create(M->getContext(), "", newF);
  IRBuilder<> IRB(B);

  // create a box for this value and fill it in the box
  Value* ai = IRB.CreateAlloca(T);
  IRB.CreateStore(&*newF->arg_begin(), ai);
  if (!T->isAggregateType()) {
    uint64_t size = DL.getTypeSizeInBits(T) >> 3;
    // the `size` arg
    auto CI = ConstantInt::get((++CF->arg_begin())->getType(), size);
    // the pointer arg
    auto P = IRB.CreateBitCast(ai, CF->arg_begin()->getType());
    IRB.CreateCall(CF, {P, CI});
  } else
    assert(false && "Doesn't support aggregate types");

  // add return
  IRB.CreateRetVoid();

  counter++;
  return newF;
}

std::vector<CallInst*> getPublicInCalls(Function* F) {
  std::vector<CallInst*> cis;
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    if (CallInst* ci = dyn_cast<CallInst>(&*I)) {
      Function* f = ci->getCalledFunction();
      if (!f)
        f = dyn_cast<Function>(ci->getCalledValue()->stripPointerCasts());
      if (f && f->hasName()) {
        if (f->getName().str() == Naming::PUBLIC_IN_FUNC) {
          assert(ci->getNumArgOperands() == 1 && "Assume only one argument to public_in calls");
          cis.push_back(ci);
        }
      }
    }
  }
  return cis;
}

void insertPublicInHandleFuncs(Module& M, Function* specF) {
  auto cis = getPublicInCalls(specF);
  std::map<CallInst*, Function*> bindings;
  for (auto ci : cis) {
    Value* arg = ci->getArgOperand(0);
    auto F = buildPublicInHandleFunc(&M, arg);
    bindings[ci] = F;
  }

  // first replace public_in calls with instrumented ones
  for (auto& binding: bindings) {
    CallInst* oc = binding.first;
    Function* f = binding.second;
    auto ci = CallInst::Create(f, {oc->getArgOperand(0)}, "", oc);
    oc->replaceAllUsesWith(ci);
  }

  // clean up
  for (auto& binding: bindings)
    binding.first->eraseFromParent();
  getFunction(M, Naming::PUBLIC_IN_FUNC)->eraseFromParent();
}

inline void createCallToRWF(IRBuilder<>& IRB, Function* RWF, Value* A, Value* CI) {
  auto P = IRB.CreateBitCast(A, RWF->arg_begin()->getType());
  IRB.CreateCall(RWF, {P, CI});
}

argInfo readWriteValue(IRBuilder<>& IRB, Module& M, Function* RWF, Value* arg) {
  Type* T = arg->getType();
  auto DL = M.getDataLayout();
  uint64_t size = DL.getTypeStoreSize(T);
  auto CI = ConstantInt::get((++RWF->arg_begin())->getType(), size);
  auto A = IRB.CreateAlloca(T);
  createCallToRWF(IRB, RWF, A, CI);
  return std::make_pair(A, CI);
}

Function* buildReadAndCheckFunc(Module& M, Function* specF) {
  Function* F = getFunction(M, "__ct_fuzz_read_and_check_inputs");
  Function* stdinRF = getFunction(M, "__ct_fuzz_stdin_read");
  Function* pipeWF = getFunction(M, "__ct_fuzz_input_pipe_write");
  Function* sCheckF = getFunction(M, "__ct_fuzz_input_check_start");


  BasicBlock* B = BasicBlock::Create(M.getContext(), "", F);
  IRBuilder<> IRB(B);

  // create two copies of input variables and read values into them
  std::vector<argInfoP> vps;
  for (auto& arg : specF->args()) {
    vps.push_back(std::make_pair(readWriteValue(IRB, M, stdinRF, &arg), readWriteValue(IRB, M, stdinRF, &arg)));
  }

  // run spec once
  auto GA = [&IRB](std::vector<argInfoP>& ps, unsigned char count) -> std::vector<Value*> {
    // what is this
    std::vector<Value*> vs;
    for(auto& p : ps) {
      Value* A = count? p.second.first : p.first.first;
      vs.push_back(IRB.CreateLoad(A));
    }
    return vs;
  };

  IRB.CreateCall(specF, GA(vps, 0));
  // change to check mode
  IRB.CreateCall(sCheckF, {});
  // run spec twice
  IRB.CreateCall(specF, GA(vps, 1));

  // write input set one to the pipe
  for (auto& v : vps) {
    auto P = v.first.first;
    auto C = v.first.second;
    createCallToRWF(IRB, pipeWF, P, C);
  }
  for (auto& v : vps) {
    auto P = v.second.first;
    auto C = v.second.second;
    createCallToRWF(IRB, pipeWF, P, C);
  }

  // create ret
  IRB.CreateRetVoid();
  return F;
}

void buildExecFunc(Module& M, Function* srcF) {
  // read input from the pipe
  Function* F = getFunction(M, "__ct_fuzz_exec");
  Function* pipeRF = getFunction(M, "__ct_fuzz_input_pipe_read");


  BasicBlock* B = BasicBlock::Create(M.getContext(), "", F);
  IRBuilder<> IRB(B);

  std::vector<Value*> newArgs;
  for (auto& arg : srcF->args()) {
    auto A = readWriteValue(IRB, M, pipeRF, &arg).first;
    newArgs.push_back(IRB.CreateLoad(A));
  }

  // call src function
  IRB.CreateCall(srcF, newArgs);

  IRB.CreateRetVoid();
}

Function* getSpecFunction(Module& M) {
  for (auto &F : M.functions()) {
    if (F.hasName() && F.getName().find(Naming::SPEC_FUNC_PREFIX) == 0)
      return &F;
  }
  llvm_unreachable("Unable to find spec function.");
}

bool CTFuzzInstrument::runOnModule(Module& M) {
  Function* srcF = getFunction(M, CTFuzzOptions::EntryPoint);
  Function* specF = getSpecFunction(M);
  insertPublicInHandleFuncs(M, specF);

  buildReadAndCheckFunc(M, specF);
  buildExecFunc(M, srcF);

  return false;
}

// Pass ID variable
char CTFuzzInstrument::ID = 0;

// Register the pass
static RegisterPass<CTFuzzInstrument>
X("ct-fuzz", "Instrumentations for constant time fuzzer");
