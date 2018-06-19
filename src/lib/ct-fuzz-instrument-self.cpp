#include "ct-fuzz-instrument-self.h"

using namespace llvm;
typedef CTFuzzInstrumentUtils Utils;

CallInst* getCallToReadInputsFunc(Function* mainF) {
  return Utils::getCallToFuncOnce(mainF, "__ct_fuzz_read_inputs");
}

CallInst* getCallToSpecFunc(Function* mainF) {
  return Utils::getCallToFuncOnce(mainF, "__ct_fuzz_spec");
}

CallInst* getCallToExecFunc(Function* mainF) {
  return Utils::getCallToFuncOnce(mainF, "__ct_fuzz_exec");
}

std::vector<CallInst*> getPublicInCalls(Function* F) {
  return Utils::getCallFromFunc(F, Naming::PUBLIC_IN_FUNC);
}

Function* CTFuzzInstrumentSelf::buildPublicInHandleFunc(Module* M, CallInst* CI) {
  static unsigned counter = 0;
  auto DL = M->getDataLayout();
  Type* sizeT = Utils::getFirstArg(mallocF)->getType();
  Type* T = CI->getArgOperand(0)->getType();
  std::vector<Type*> ts;

  Function* CF = Utils::getFunction(*M, Naming::PUBLIC_IN_HANDLE_FUNC);

  for(unsigned i = 0; i < CI->getNumArgOperands(); ++i)
    ts.push_back(CI->getArgOperand(i)->getType());

  FunctionType* newFT = FunctionType::get(Type::getVoidTy(M->getContext()), ts, false);
  std::string newFN = Naming::PUBLIC_IN_FUNC + "_" + std::to_string(counter);
  Function* newF = Function::Create(newFT, GlobalValue::InternalLinkage, newFN, M);

  BasicBlock* B = BasicBlock::Create(M->getContext(), "", newF);
  IRBuilder<> IRB(B);

  // the pointer arg of public value handle function
  Value* P = nullptr;
  // the size arg
  Value* S = nullptr;
  assert(!T->isAggregateType() && "Doesn't support aggregate types");

  if (!T->isPointerTy()) {
    // if the argument is not a pointer, create an allocated space,
    // store its value in, and pass the pointer to the allocated space
    P = IRB.CreateAlloca(T);
    IRB.CreateStore(Utils::getFirstArg(newF), P);
    // FIXME: I'm not sure if GetTypeSizeInBits is proper here
    uint64_t size = DL.getTypeSizeInBits(T) >> 3;
    S = ConstantInt::get(Utils::getSecondArg(CF)->getType(), size);
  } else {
    P = Utils::getFirstArg(newF);
    // note that the second argument for __ct_fuzz_public_in function is
    // the number of elements in this array
    S = Utils::getByteSizeInSizeT(IRB, DL,
      Utils::getSecondArg(newF), cast<PointerType>(T)->getElementType(), sizeT);
  }

  IRB.CreateCall(CF, {IRB.CreateBitCast(P, CF->arg_begin()->getType()), S});

  IRB.CreateRetVoid();

  counter++;
  return newF;
}

void CTFuzzInstrumentSelf::insertPublicInHandleFuncs(Module& M, Function* specF) {
  auto cis = getPublicInCalls(specF);
  std::map<CallInst*, Function*> bindings;

  // note that we create a new function for each call site
  // which could be implemented in a per-type way
  // for the simplicity of the implementations, I don't do it
  for (auto ci : cis) {
    auto F = buildPublicInHandleFunc(&M, ci);
    bindings[ci] = F;
  }

  // replace __ct_fuzz_public_in calls with instrumented ones
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

inline AllocaInst* createArray(IRBuilder<>& IRB, Type* T) {
  return IRB.CreateAlloca(ArrayType::get(T, 2));
}

inline ConstantInt* createIdx(Value* AI, idx_t idx) {
  return ConstantInt::get(IntegerType::get(AI->getContext(), 64), idx);
}

inline Value* getElement(IRBuilder<>& IRB, AllocaInst* AI, idx_t idx) {
  return IRB.CreateGEP(AI, {createIdx(AI, 0), createIdx(AI, idx)});
}

inline Value* getElement(IRBuilder<>& IRB, AllocaInst* AI, Value* idx) {
  return IRB.CreateGEP(AI, {createIdx(AI, 0), idx});
}

CallInst* CTFuzzInstrumentSelf::readInputs(Module& M, Function* mainF, Function* srcF, BoxesList& boxes) {
  CallInst* TCI = getCallToReadInputsFunc(mainF);
  Function* stdinRF = Utils::getFunction(M, "__ct_fuzz_stdin_read");
  Type* sizeT = Utils::getSecondArg(stdinRF)->getType();
  LLVMContext& C = M.getContext();

  IRBuilder<> IRB(TCI);

  // create two copies of input variables and read values into them
  for (auto& arg : srcF->args()) {
    auto elemArr = createArray(IRB, arg.getType());
    AllocaInst* lenBox = nullptr;
    if (arg.getType()->isPointerTy())
      lenBox = createArray(IRB, Utils::getLenT(C));
    boxes.push_back(std::make_pair(elemArr, lenBox));
  }

  // encoding of pointer arguments
  // the first two bytes are element length (*not byte size*)
  // then read (element size * element length) bytes
  auto GI = [&IRB, &M, &stdinRF, &srcF, &boxes, &sizeT, this](idx_t idx) -> void {
    unsigned i = 0;
    LLVMContext& C = M.getContext();
    auto DL = M.getDataLayout();

    for (auto& arg : srcF->args()) {
      Type* T = arg.getType();
      Type* ptrArgT = Utils::getFirstArg(stdinRF)->getType();
      bool isPtrTy = T->isPointerTy();
      // when the argument is not pointer type, just create a box to hold the two values
      if (!isPtrTy) {
        auto size = Utils::getTypeSizeInSizeT(DL, T, sizeT);
        auto ptr = getElement(IRB, boxes[i].first, idx);
        IRB.CreateCall(stdinRF, {IRB.CreateBitCast(ptr, ptrArgT), size});
      } else {
        // when the argument is pointer type, create two boxes:
        // one for lengths
        // the other for pointers
        auto lenP = getElement(IRB, boxes[i].second, idx);
        auto lenS = Utils::getTypeSizeInSizeT(DL, Utils::getLenT(C), sizeT);
        IRB.CreateCall(stdinRF, {IRB.CreateBitCast(lenP, ptrArgT), lenS});
        auto sizeV = Utils::getByteSizeInSizeT(IRB, DL, IRB.CreateLoad(lenP),
          cast<PointerType>(T)->getElementType(), sizeT);
        CallInst* mallocC = IRB.CreateCall(mallocF, {sizeV});
        IRB.CreateCall(stdinRF, {IRB.CreateBitCast(mallocC, ptrArgT), sizeV});
        IRB.CreateStore(IRB.CreateBitCast(mallocC, T), getElement(IRB, boxes[i].first, idx));
      }
      ++i;
    }
  };

  GI(0); GI(1);

  return TCI;
}

Function* CTFuzzInstrumentSelf::buildNewSpecFunc(Module& M, Function* specF) {
  std::vector<Type*> newTs;
  std::map<Value*, Value*> ptrArgMap;
  ValueToValueMapTy VMap;
  LLVMContext& C = M.getContext();
  SmallVector<ReturnInst*, 8> Returns;

  for (auto& arg : specF->args()) {
    Type* T = arg.getType();
    newTs.push_back(T);
    if (T->isPointerTy())
      newTs.push_back(Utils::getLenT(C));
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

  auto findMapping = [&ptrArgMap](Value* ptrArg) -> Value* {
    if (auto li = dyn_cast<LoadInst>(ptrArg)) {
      if (auto ai = dyn_cast<AllocaInst>(li->getPointerOperand()->stripPointerCasts())) {
        for (auto u : ai->users()) {
          if (auto si = dyn_cast<StoreInst>(u))
            return ptrArgMap[si->getValueOperand()];
        }
        llvm_unreachable("weird.");
      }
    }
      return ptrArgMap[ptrArg];
  };
  // search array length function calls and replace them with length arg
  auto CIS = Utils::getCallFromFunc(NewF, "__ct_fuzz_array_len");
  for (auto &CI : CIS) {
    IRBuilder<> IRB(CI);
    Value* P = CI->getArgOperand(0)->stripPointerCasts();
    Value* V = findMapping(P);
    CI->replaceAllUsesWith(V);
  }
  for (auto &CI : CIS)
    CI->eraseFromParent();
  return NewF;
}

CallInst* CTFuzzInstrumentSelf::checkInputs(Module& M, Function* mainF, Function* specF, const BoxesList& boxes) {
  CallInst* TCI = getCallToSpecFunc(mainF);
  IRBuilder<> IRB(TCI);

  Value* idx = IRB.CreateZExt(TCI->getOperand(0), IntegerType::get(TCI->getContext(), 64));
  unsigned i = 0;
  std::vector<Value*> args;

  for (auto& arg : specF->args()) {
    Type* T = arg.getType();
    args.push_back(IRB.CreateLoad(getElement(IRB, boxes[i].first, idx)));
    if (T->isPointerTy())
      args.push_back(IRB.CreateLoad(getElement(IRB, boxes[i].second, idx)));
    ++i;
  }

  IRB.CreateCall(buildNewSpecFunc(M, specF), args);

  specF->eraseFromParent();
  return TCI;
}

// NOTE: we're in a forked process
CallInst* CTFuzzInstrumentSelf::execInputFunc(Module& M, Function* mainF, Function* srcF, const BoxesList& boxes) {
  CallInst* TCI = getCallToExecFunc(mainF);
  Function* maxF = Utils::getFunction(M, "__ct_fuzz_max_len");
  Type* sizeT = Utils::getFirstArg(mallocF)->getType();
  auto DL = M.getDataLayout();
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
      Type* ET = cast<PointerType>(T)->getElementType();
      auto maxLen = Utils::getByteSizeInSizeT(IRB, DL, lens[len_i], ET, sizeT);
      // TODO: initialize this chunk of memory?
      auto mallocCI = IRB.CreateCall(mallocF, {maxLen});
      auto sizeV = Utils::getByteSizeInSizeT(IRB, DL,
        IRB.CreateLoad(getElement(IRB, lenPP, idx)), ET, sizeT);
      auto valueP = IRB.CreateLoad(getElement(IRB, valuePP, idx));
      IRB.CreateCall(memcpyF, {IRB.CreateBitCast(mallocCI, Utils::getFirstArg(memcpyF)->getType()),
          IRB.CreateBitCast(valueP, Utils::getSecondArg(memcpyF)->getType()), sizeV});
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
  Function* srcF = Utils::getFunction(M, CTFuzzOptions::EntryPoint);
  Function* specF = getSpecFunction(M);
  Function* mainF = Utils::getFunction(M, "__ct_fuzz_main");

  // two heavily used functions
  mallocF = Utils::getFunction(M, "__ct_fuzz_malloc_wrapper");
  memcpyF = Utils::getFunction(M, "__ct_fuzz_memcpy_wrapper");

  // this function instruments __ct_fuzz_public_in calls such that
  // the value passed to the these calls are either recorded or checked
  // via calling public_value_handle functions
  insertPublicInHandleFuncs(M, specF);

  // boxes is a pair of pointers
  // the first element is a pointer to the function argument
  // the second element is a pointer to the element length
  // if the argument is not a pointer, then the second element is nullptr
  BoxesList boxes;
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
