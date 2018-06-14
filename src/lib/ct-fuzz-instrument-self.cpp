#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "ct-fuzz-instrument-self.h"
#include "ct-fuzz-naming.h"
#include "ct-fuzz-options.h"
#include "ct-fuzz-instrument-utils.h"

#include <utility>
#include <vector>
#include <map>

using namespace llvm;

typedef unsigned char idx_t;
typedef std::pair<AllocaInst*, AllocaInst*> Boxes;
typedef std::vector<Boxes> BoxesList;

CallInst* getCallToReadInputsFunc(Function* mainF) {
  return CTFuzzInstrumentUtils::getCallToFuncOnce(mainF, "__ct_fuzz_read_inputs");
}

CallInst* getCallToSpecFunc(Function* mainF) {
  return CTFuzzInstrumentUtils::getCallToFuncOnce(mainF, "__ct_fuzz_spec");
}

CallInst* getCallToExecFunc(Function* mainF) {
  return CTFuzzInstrumentUtils::getCallToFuncOnce(mainF, "__ct_fuzz_exec");
}

std::vector<CallInst*> getPublicInCalls(Function* F) {
  return CTFuzzInstrumentUtils::getCallFromFunc(F, Naming::PUBLIC_IN_FUNC);
}

Function* buildPublicInHandleFunc(Module* M, CallInst* CI) {
  static unsigned counter = 0;
  auto DL = M->getDataLayout();
  Type* T = CI->getArgOperand(0)->getType();
  std::vector<Type*> ts;

  Function* CF = CTFuzzInstrumentUtils::getFunction(*M, Naming::PUBLIC_IN_HANDLE_FUNC);

  for(unsigned i = 0; i < CI->getNumArgOperands(); ++i)
    ts.push_back(CI->getArgOperand(i)->getType());

  FunctionType* newFT = FunctionType::get(Type::getVoidTy(M->getContext()), ts, false);
  std::string newFN = Naming::PUBLIC_IN_FUNC + "_" + std::to_string(counter);
  Function* newF = Function::Create(newFT, GlobalValue::InternalLinkage, newFN, M);

  BasicBlock* B = BasicBlock::Create(M->getContext(), "", newF);
  IRBuilder<> IRB(B);

  Value* P = nullptr;
  Value* S = nullptr;
  assert(!T->isAggregateType() && "Doesn't support aggregate types");

  if (!T->isPointerTy()) {
    P = IRB.CreateAlloca(T);
    IRB.CreateStore(&*newF->arg_begin(), P);
    uint64_t size = DL.getTypeSizeInBits(T) >> 3;
    S = ConstantInt::get(CTFuzzInstrumentUtils::getSecondArg(CF)->getType(), size);
    P = IRB.CreateBitCast(P, CF->arg_begin()->getType());
  } else {
    P = CTFuzzInstrumentUtils::getFirstArg(newF);
    S = IRB.CreateZExtOrBitCast(CTFuzzInstrumentUtils::getSecondArg(newF), CTFuzzInstrumentUtils::getSecondArg(CF)->getType());
  }

  IRB.CreateCall(CF, {P, S});

  IRB.CreateRetVoid();

  counter++;
  return newF;
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
  return IRB.CreateAlloca(ArrayType::get(T, 2));
}

inline ConstantInt* createIdx(Value* AI, idx_t idx) {
  return ConstantInt::get(IntegerType::get(AI->getContext(), 64), idx);
}

inline Value* getElement(IRBuilder<>& IRB, AllocaInst* AI, idx_t idx, Type* T = nullptr) {
  return IRB.CreateGEP(AI, {createIdx(AI, 0), createIdx(AI, idx)});
}

inline Value* getElement(IRBuilder<>& IRB, AllocaInst* AI, Value* idx, Type* T = nullptr) {
  return IRB.CreateGEP(AI, {createIdx(AI, 0), idx});
}

CallInst* readInputs(Module& M, Function* mainF, Function* srcF, BoxesList& boxes) {
  CallInst* TCI = getCallToReadInputsFunc(mainF);
  Function* stdinRF = CTFuzzInstrumentUtils::getFunction(M, "__ct_fuzz_stdin_read");
  Function* mallocF = CTFuzzInstrumentUtils::getFunction(M, "__ct_fuzz_malloc_wrapper");
  Type* sizeT = CTFuzzInstrumentUtils::getSecondArg(stdinRF)->getType();

  IRBuilder<> IRB(TCI);

  // create two copies of input variables and read values into them
  for (auto& arg : srcF->args()) {
    auto elemArr = createArray(IRB, arg.getType());
    AllocaInst* lenBox = nullptr;
    if (arg.getType()->isPointerTy())
      lenBox = createArray(IRB, sizeT);
    boxes.push_back(std::make_pair(elemArr, lenBox));
  }

  // WARNING: should read multiple elements but it's hard to find these info ahead of time
  auto GI = [&IRB, &M, &stdinRF, &srcF, &boxes, &sizeT, &mallocF](idx_t idx) -> void {
    unsigned i = 0;
    for (auto& arg : srcF->args()) {
      Type* T = arg.getType();
      bool isPtrTy = T->isPointerTy();
      // when the argument is not pointer type, just create a box to hold the two values
      if (!isPtrTy) {
        auto size = ConstantInt::get(sizeT, M.getDataLayout().getTypeStoreSize(T));
        createCallToRWF(IRB, stdinRF, getElement(IRB, boxes[i].first, idx, T), size);
      }
      else {
        // when the argument is pointer type, create two boxes:
        // one for lengths
        // the other for pointers
        auto lenP = getElement(IRB, boxes[i].second, idx, sizeT);
        createCallToRWF(IRB, stdinRF, lenP, ConstantInt::get(sizeT, M.getDataLayout().getTypeStoreSize(sizeT)));
        auto lenV = IRB.CreateLoad(lenP);
        CallInst* mallocC = IRB.CreateCall(mallocF, {lenV});
        createCallToRWF(IRB, stdinRF, mallocC, lenV);
        IRB.CreateStore(IRB.CreateBitCast(mallocC, T), getElement(IRB, boxes[i].first, idx, T));
      }
      ++i;
    }
  };

  GI(0); GI(1);

  return TCI;
}

Function* buildNewSpecFunc(Module& M, Function* specF) {
  std::vector<Type*> newTs;
  std::map<Value*, Value*> ptrArgMap;
  ValueToValueMapTy VMap;
  // FIXME: non-portable here
  Type* sizeT = IntegerType::get(M.getContext(), 64);
  SmallVector<ReturnInst*, 8> Returns;

  for (auto& arg : specF->args()) {
    Type* T = arg.getType();
    newTs.push_back(T);
    if (T->isPointerTy())
      newTs.push_back(sizeT);
  }
  
  FunctionType* NT = FunctionType::get(specF->getReturnType(), newTs, false);
  Function* NewF = Function::Create(NT, specF->getLinkage(), specF->getName()+"_new", &M);

  Function::arg_iterator DestA = NewF->arg_begin();
  for (auto &A : specF->args()) {
    VMap[&A] = &*DestA;
    if (A.getType()->isPointerTy()) {
      auto temp = DestA;
      ptrArgMap[&*temp] = &*++DestA;
    }
    ++DestA;
  }
  CloneFunctionInto(NewF, specF, VMap, false, Returns);

  // search array length function calls and replace them with length arg
  auto CIS = CTFuzzInstrumentUtils::getCallFromFunc(NewF, "__ct_fuzz_array_len");
  for (auto &CI : CIS) {
    IRBuilder<> IRB(CI);
    Value* P = CI->getArgOperand(0)->stripPointerCasts();
    Type* ET = cast<PointerType>(P->getType())->getElementType();
    CI->replaceAllUsesWith(
      IRB.CreateUDiv(ptrArgMap[P], ConstantInt::get(sizeT, M.getDataLayout().getTypeStoreSize(ET))));
  }
  for (auto &CI : CIS)
    CI->eraseFromParent();
  return NewF;
}

CallInst* checkInputs(Module& M, Function* mainF, Function* specF, const BoxesList& boxes) {
  CallInst* TCI = getCallToSpecFunc(mainF);
  IRBuilder<> IRB(TCI);

  Value* idx = IRB.CreateZExt(TCI->getOperand(0), IntegerType::get(TCI->getContext(), 64));
  unsigned i = 0;
  std::vector<Value*> args;

  for (auto& arg : specF->args()) {
    Type* T = arg.getType();
    args.push_back(IRB.CreateLoad(getElement(IRB, boxes[i].first, idx, T)));
    if (T->isPointerTy())
      args.push_back(IRB.CreateLoad(getElement(IRB, boxes[i].second, idx)));
    ++i;
  }

  IRB.CreateCall(buildNewSpecFunc(M, specF), args);

  specF->eraseFromParent();
  return TCI;
}

// NOTE: we're in a forked process
CallInst* execInputFunc(Module& M, Function* mainF, Function* srcF, const BoxesList& boxes) {
  CallInst* TCI = getCallToExecFunc(mainF);
  Function* maxF = CTFuzzInstrumentUtils::getFunction(M, "__ct_fuzz_size_t_max");
  Function* memcpyF = CTFuzzInstrumentUtils::getFunction(M, "__ct_fuzz_memcpy_wrapper");
  IRBuilder<> IRB(TCI);

  Value* idx = IRB.CreateZExt(TCI->getOperand(0), IntegerType::get(TCI->getContext(), 64));
  std::vector<Value*> args;
  std::vector<Value*> lens;

  for (auto& p : boxes) {
    auto lenP = p.second;
    if (lenP)
      lens.push_back(IRB.CreateCall(maxF,
        {IRB.CreateLoad(getElement(IRB, lenP, (idx_t)0)),
          IRB.CreateLoad(getElement(IRB, lenP, (idx_t)1))}));
  }

  unsigned len_i = 0;
  unsigned i = 0;
  for (auto& arg : srcF->args()) {
    Type* T = arg.getType();
    bool isPtrTy = T->isPointerTy();
    auto valuePP = boxes[i].first;
    auto lenPP = boxes[i].second;
    if (!isPtrTy) {
      args.push_back(IRB.CreateLoad(getElement(IRB, valuePP, idx)));
    } else {
      // read malloc length first
      auto maxLen = lens[len_i];
      // call malloc
      // TODO: initialize this chunk of memory
      auto mallocCI = IRB.CreateCall(CTFuzzInstrumentUtils::getFunction(M, "malloc"), {maxLen});
      auto len = IRB.CreateLoad(getElement(IRB, lenPP, idx));
      auto valueP = IRB.CreateLoad(getElement(IRB, valuePP, idx));
      IRB.CreateCall(memcpyF, {IRB.CreateBitCast(mallocCI, CTFuzzInstrumentUtils::getFirstArg(memcpyF)->getType()),
          IRB.CreateBitCast(valueP, CTFuzzInstrumentUtils::getSecondArg(memcpyF)->getType()), len});
      args.push_back(IRB.CreateBitCast(mallocCI, T));
      ++len_i;
    }
    ++i;
  }

  // call the function
  IRB.CreateCall(srcF, args);

  return TCI;
}

Function* getSpecFunction(Module& M) {
  for (auto &F : M.functions()) {
    if (F.hasName() && F.getName().find(Naming::SPEC_FUNC_PREFIX) == 0)
      return &F;
  }
  llvm_unreachable("Unable to find spec function.");
}

bool CTFuzzInstrumentSelf::runOnModule(Module& M) {
  Function* srcF = CTFuzzInstrumentUtils::getFunction(M, CTFuzzOptions::EntryPoint);
  Function* specF = getSpecFunction(M);
  Function* mainF = CTFuzzInstrumentUtils::getFunction(M, "__ct_fuzz_main");

  BoxesList boxes;
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
char CTFuzzInstrumentSelf::ID = 0;

// Register the pass
static RegisterPass<CTFuzzInstrumentSelf>
X("ct-fuzz-instrument-self", "Instrumentations for constant time fuzzer");
