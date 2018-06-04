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

Function* buildPublicInHandleFunc(Module* M, CallInst* CI) {
  static unsigned counter = 0;
  auto DL = M->getDataLayout();
  //auto nArgs = CI->getNumOperands();
  Type* T = CI->getArgOperand(0)->getType();
  std::vector<Type*> ts;

  Function* CF = getFunction(*M, Naming::PUBLIC_IN_HANDLE_FUNC);

  for(unsigned i = 0; i < CI->getNumArgOperands(); ++i)
    ts.push_back(CI->getArgOperand(i)->getType());

  FunctionType* newFT = FunctionType::get(Type::getVoidTy(M->getContext()), ts, false);
  std::string newFN = Naming::PUBLIC_IN_FUNC + "_" + std::to_string(counter);
  Function* newF = Function::Create(newFT, GlobalValue::InternalLinkage, newFN, M);

  BasicBlock* B = BasicBlock::Create(M->getContext(), "", newF);
  IRBuilder<> IRB(B);

  // create a box for this value and fill it in the box
  //Value* P = IRB.CreateAlloca(T);
  Value* P = nullptr;
  Value* S = nullptr;
  assert(!T->isAggregateType() && "Doesn't support aggregate types");

  if (!T->isPointerTy()) {
    P = IRB.CreateAlloca(T);
    IRB.CreateStore(&*newF->arg_begin(), P);
    uint64_t size = DL.getTypeSizeInBits(T) >> 3;
    S = ConstantInt::get((++CF->arg_begin())->getType(), size);
    P = IRB.CreateBitCast(P, CF->arg_begin()->getType());
  } else {
    P = CI->getArgOperand(0);
    S = CI->getArgOperand(1);
  }

  IRB.CreateCall(CF, {P, S});

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
          assert((ci->getNumArgOperands() == 1 || ci->getNumArgOperands() == 2) && "Assume only one or two arguments to public_in calls");
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
    auto F = buildPublicInHandleFunc(&M, ci);
    bindings[ci] = F;
  }

  // first replace public_in calls with instrumented ones
  for (auto& binding: bindings) {
    CallInst* oc = binding.first;
    Function* f = binding.second;
    std::vector<Value*> args;
    for (unsigned i = 0; i < oc->getNumArgOperands(); ++i)
      args.push_back(oc->getArgOperand(i));
    auto ci = CallInst::Create(f, args, "", oc);
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

Value* allocateAndReadChunk(IRBuilder<>& IRB, Module& M, Function* RWF, Value* arg) {
  auto mallocF = getFunction(M, "malloc");
  auto lenT = mallocF->arg_begin()->getType();
  auto len = IRB.CreateLoad(readWriteValue(IRB, M, RWF, ConstantInt::get(lenT, 0)).first);
  CallInst* mallocC = IRB.CreateCall(mallocF, {len});
  createCallToRWF(IRB, RWF, mallocC, len);
  return len;
}

Function* buildReadAndCheckFunc(Module& M, Function* specF) {
  Function* F = getFunction(M, "__ct_fuzz_read_and_check_inputs");
  Function* stdinRF = getFunction(M, "__ct_fuzz_stdin_read");
  Function* iPipeWF = getFunction(M, "__ct_fuzz_input_pipe_write");
  Function* cPipeWF = getFunction(M, "__ct_fuzz_cmd_pipe_write");
  Function* sCheckF = getFunction(M, "__ct_fuzz_input_check_start");
  Function* maxF = getFunction(M, "__ct_fuzz_size_t_max");


  BasicBlock* B = BasicBlock::Create(M.getContext(), "", F);
  IRBuilder<> IRB(B);

  // create two copies of input variables and read values into them
  // broken should read values instance by instance
  std::vector<argInfo> vp1;
  std::vector<argInfo> vp2;
  std::vector<Value*> dlen1;
  std::vector<Value*> dlen2;
  auto GI = [&IRB, &specF, &M, &stdinRF](std::vector<argInfo>& vp, std::vector<Value*>& dlen) -> void {
    for (auto& arg : specF->args()) {
      if (!arg.getType()->isPointerTy())
        vp.push_back(readWriteValue(IRB, M, stdinRF, &arg));
      else {
        auto len = allocateAndReadChunk(IRB, M, stdinRF, &arg);
        dlen.push_back(len);
        vp.push_back(std::make_pair(&arg, len));
      }
    }
  };
  GI(vp1, dlen1); GI(vp2, dlen2);
  assert(vp1.size() == vp2.size() && dlen1.size() == dlen2.size());

  auto GA = [&IRB](std::vector<argInfo>& ps) -> std::vector<Value*> {
    std::vector<Value*> args;
    for(unsigned i = 0; i < ps.size(); ++i) {
      auto V = ps[i].first;
      auto S = ps[i].second;
      if (isa<ConstantInt>(S))
        args.push_back(IRB.CreateLoad(V));
      else
        args.push_back(V);
    }
    return args;
  };
  // run spec once
  IRB.CreateCall(specF, GA(vp1));
  // change to check mode
  IRB.CreateCall(sCheckF, {});
  // run spec twice
  IRB.CreateCall(specF, GA(vp2));

  // write input set one to the pipe
  auto WIP = [&IRB, &iPipeWF](std::vector<argInfo>& vp) -> void {
    for (auto& v : vp) {
      auto P = v.first;
      auto C = v.second;
      createCallToRWF(IRB, iPipeWF, P, C);
    }
  };
  WIP(vp1); WIP(vp2);

  // write maximum size to the command pipe
  auto WCP = [&IRB, &cPipeWF, &dlen1, &dlen2, &maxF]() -> void {
    for (unsigned i = 0; i < dlen1.size(); ++i) {
      auto dl1 = dlen1[i];
      auto dl2 = dlen2[i];
      IRB.CreateCall(cPipeWF, {IRB.CreateCall(maxF, {dl1, dl2})});
    }
  };
  WCP(); WCP();

  // create ret
  IRB.CreateRetVoid();
  return F;
}

void buildExecFunc(Module& M, Function* srcF) {
  // read input from the pipe
  Function* F = getFunction(M, "__ct_fuzz_exec");
  Function* iPipeRF = getFunction(M, "__ct_fuzz_input_pipe_read");
  Function* cPipeWF = getFunction(M, "__ct_fuzz_cmd_pipe_read");

  BasicBlock* B = BasicBlock::Create(M.getContext(), "", F);
  IRBuilder<> IRB(B);

  std::vector<Value*> newArgs;
  for (auto& arg : srcF->args()) {
    if (!arg.getType()->isPointerTy()) {
      auto A = readWriteValue(IRB, M, iPipeRF, &arg).first;
      newArgs.push_back(IRB.CreateLoad(A));
    } else {
      // read malloc length first
      auto aLen = IRB.CreateCall(cPipeWF, {});
      // call malloc
      // TODO: initialize this chunk of memory
      auto mi = IRB.CreateCall(getFunction(M, "malloc"), {aLen});
      // read length in
      auto dLen = readWriteValue(IRB, M, iPipeRF, &arg).first;
      // read value in
      createCallToRWF(IRB, iPipeRF, mi, dLen);
    }
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
