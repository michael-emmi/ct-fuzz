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
typedef unsigned char idx_t;

Function* getFunction(Module& M, std::string name) {
  Function* f = M.getFunction(name);
  assert(f && "Unable to get function in this module.");
  return f;
}

inline Value* getSecondArg(Function* F) {
  auto I = F->arg_begin();
  return ++I;
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
    S = ConstantInt::get(getSecondArg(CF)->getType(), size);
    P = IRB.CreateBitCast(P, CF->arg_begin()->getType());
  } else {
    P = newF->arg_begin();
    S = IRB.CreateZExtOrBitCast(getSecondArg(newF), getSecondArg(CF)->getType());
  }

  IRB.CreateCall(CF, {P, S});

  // add return
  IRB.CreateRetVoid();

  counter++;
  return newF;
}

std::vector<CallInst*> getCallFromFunc(Function* F, std::string FN) {
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

std::vector<CallInst*> getPublicInCalls(Function* F) {
  getCallFromFunc(F, Naming::PUBLIC_IN_FUNC);
}

CallInst* getCallToFuncOnce(Function* mainF, std::string FN) {
  auto ret = getCallFromFunc(mainF, FN);
  assert(ret.size() == 1 && "Should see just one call site.");
  return ret[0];
}

CallInst* getCallToReadInputsFunc(Function* mainF) {
  return getCallFromFuncOnce(mainF, "__ct_fuzz_read_inputs");
}

CallInst* getCallToSpecFunc(Function* mainF) {
  return getCallFromFuncOnce(mainF, "__ct_fuzz_spec");
}

CallInst* getCallToSpecFunc(Function* mainF) {
  return getCallFromFuncOnce(mainF, "__ct_fuzz_exec");
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
  auto PF = M.getFunction(Naming::PUBLIC_IN_FUNC);
  if (PF)
    PF->eraseFromParent();
}

inline void createCallToRWF(IRBuilder<>& IRB, Function* RWF, Value* A, Value* CI) {
  auto P = IRB.CreateBitCast(A, RWF->arg_begin()->getType());
  IRB.CreateCall(RWF, {P, CI});
}

AllocaInst* createArray(IRBuilder<>& IRB, Type* T) {
  IRB.CreateAlloca(ArrayType::get(T, 2));
}

inline ConstantInt* createIdx(Value* AI, idx_t idx) {
  return ConstantInt::get(IntegerType::get(AI->getContext(), 64), idx);
}

inline Value* getElement(IRBuilder<>& IRB, AllocaInst* AI, idx_t idx, Type* T = nullptr) {
  return IRB.CreateGEP(AI, {createIdx(AI, 0), createIdx(AI, idx)});
}


inline Value* getElement(IRBuilder<>& IRB, AllocaInst* AI, ConstantInt* idx, Type* T) {
  return IRB.CreateGEP(AI, {createIdx(AI, 0), idx});
}

CallInst* readInputs(Module& M, Function* mainF, Function* srcF, std::vector<std::pair<AllocaInst*>>& boxes) {
  CallInst* TCI = getCallToReadInputsFunc(mainF);
  Function* stdinRF = getFunction(M, "__ct_fuzz_stdin_read");
  Function* maxF = getFunction(M, "__ct_fuzz_size_t_max");
  Function* mallocF = getFunction(M, "malloc");
  Type* sizeT = mallocF->arg_begin()->getType();

  //BasicBlock* B = BasicBlock::Create(M.getContext(), "", F);
  IRBuilder<> IRB(TCI);

  // create two copies of input variables and read values into them

  std::vector<std::pair<AllocaInst*>> boxes;
  for (auto& arg : srcF->args()) {
    auto elemArr = createArray(IRB, arg.getType());
    auto lenBox = nullptr;
    if (arg.getType()->isPointerTy())
      lenBox = createArray(IRB, sizeT);
    boxes.push_back(std::make_pair(elemArr, lenBox));
  }

  // WARNING: should read multiple elements but it's hard to find these info ahead of time
  auto GI = [&IRB, &M, &stdinRF](idx_t idx) -> void {
    unsigned i = 0;
    for (auto& arg : srcF->args()) {
      Type* T = arg.getType();
      bool isPtrTy = T->isPointerTy();
      if (!isPtrTy) {
        auto size = ConstantInt::get(getSecondArg(stdinRF)->getType(), M.getDataLayout().getTypeStoreSize(T));
        createCallToRWF(IRB, stdinRF, getElement(IRB, boxes[i].first, idx, T), size);
      }
      else {
        auto lenP = getElement(IRB, boxes[i].second, idx, sizeT);
        createCallToRWF(IRB, stdinRF, , ConstantInt::get(sizeT, M.getDataLayout().getTypeStoreSize(sizeT)));
        auto lenV = IRB.CreateLoad(lenP);
        CallInst* mallocC = IRB.CreateCall(mallocF, {lenV});
        createCallToRWF(IRB, stdinRF, mallocC, lenV);
        IRB.CreateStore(IRB.createBitCast(mallocC, T), getElement(IRB, boxes[i].first, idx, T));
      }
      ++i;
    }
  };

  GI(0); GI(1);

  return TCI;
}

CallInst* checkInputs(Module& M, Function* mainF, Function* specF, const std::vector<std::pair<AllocaInst*>>& boxes) {
  CallInst* TCI = getCallToSpecFunc(mainF);
  IRBuilder<> IRB(TCI);

  Value* idx = IRB.CreateZExt(TCI->getOperand(0), IntegerType::get(TCI->getContext(), 64));
  unsigned i = 0;
  std::vector<Value*> args;

  for (auto& arg : specF->args()) {
    Type* T = arg.getType();
    auto P = boxes[i].first;
    args.push_back(IRB.CreateLoad(getElement(IRB, P, idx, T)));
  }

  IRB.CreateCall(specF, args);

  return TCI;
}

// NOTE: we're in a forked process
CallInst* execInputFunc(Module& M, Function* mainF, Function* srcF, const std::vector<std::pair<AllocaInst*>>& boxes) {
  CallInst* TCI = getCallToExecFunc(mainF);
  Function* maxF = getFunction(M, __ct_fuzz_size_t_max);
  IRBuilder<> IRB(TCI);

  Value* idx = IRB.CreateZExt(TCI->getOperand(0), IntegerType::get(TCI->getContext(), 64));
  unsigned i = 0;
  std::vector<Value*> args;
  std::vector<Value*> lens;

  for (auto& p : boxes) {
    auto lenP = p.second();
    if (lenP)
      lens.push_back(IRB.CreateCall(maxF,
        {IRB.CreateLoad(getElement(IRB, lenP, 0)),
          IRB.CreateLoad(getElement(IRB, lenP, 1))}));
  }

  unsigned len_i = 0;
  for (auto& arg : srcF->args()) {
    Type* T = arg.getType();
    bool isPtrTy = T->isPointerTy();
    auto P = boxes[i].first;
    auto lenP = boxes[i].second;
    if (!isPtrTy) {
      args.push_back(IRB.CreateLoad(getElement(IRB, P, idx)));
    } else {
      // read malloc length first
      auto maxLen = lens[len_i];
      // call malloc
      // TODO: initialize this chunk of memory
      auto mallocCI = IRB.CreateCall(getFunction(M, "malloc"), {maxLen});
      auto len = IRB.CreateLoad(getElement(IRB, lenP, idx));
      auto memcpyF = getFunction("memcpy");
      auto PP = IRB.CreateLoad(getElement(IRB, P, idx));
      IRB.CreateCall(memcpyF, {IRB.CreateBitCast(mallocCI, memcpyF->arg_begin().getType()),
          IRB.CreateBitCast(PP, getSecondARg(memcpyF)->getType())});
      args.push_back(mallocCI);
      ++len_i;
    }
  }
  
  // call the function
  IRB.Call(srcF, args);

  return TCI;
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
  Function* mainF = getFunction(M, "__ct_fuzz_main");

  std::vector<std::pair<AllocaInst*>> boxes;
  insertPublicInHandleFuncs(M, specF);
  auto I1 = readInputs(M, mainF, srcF, boxes);
  auto I2 = checkInputs(M, mainF, specF, boxes);
  auto I3 = execInputFunc(M, mainF, srcF, boxes);

  I1->eraseFromParent();
  I2->eraseFromParent();
  I3->eraseFromParent();

  return false;
}

// Pass ID variable
char CTFuzzInstrument::ID = 0;

// Register the pass
static RegisterPass<CTFuzzInstrument>
X("ct-fuzz", "Instrumentations for constant time fuzzer");
