#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include "ct-fuzz-instrument-self.h"
#include "ct-fuzz-instrument-utils.h"
#include "ct-fuzz-options.h"

using namespace llvm;
typedef CTFuzzInstrumentUtils Utils;
typedef CTFuzzOptions Opt;

#define ERASE_TCI() TCI->eraseFromParent();

CallInst* getCallToReadInputsFunc(Function* mainF) {
  return Utils::getCallToFuncOnce(mainF, "__ct_fuzz_read_inputs");
}

CallInst* getCallToSpecFunc(Function* mainF) {
  return Utils::getCallToFuncOnce(mainF, "__ct_fuzz_spec");
}

CallInst* getCallToExecFunc(Function* mainF) {
  return Utils::getCallToFuncOnce(mainF, "__ct_fuzz_exec");
}

CallInst* getCallToMergeFunc(Function* mainF) {
  return Utils::getCallToFuncOnce(mainF, "__ct_fuzz_merge_ptr_inputs");
}

std::vector<CallInst*> getPublicInCalls(Function* F) {
  return Utils::getCallFromFunc(F, Naming::PUBLIC_IN_FUNC);
}

Function* CTFuzzInstrumentSelf::buildPublicInHandleFunc(CallInst* CI) {
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

void CTFuzzInstrumentSelf::insertPublicInHandleFuncs(Function* specF) {
  auto cis = getPublicInCalls(specF);
  std::map<CallInst*, Function*> bindings;

  // note that we create a new function for each call site
  // which could be implemented in a per-type way
  // for the simplicity of the implementations, I don't do it
  for (auto ci : cis) {
    auto F = buildPublicInHandleFunc(ci);
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
  auto PF = M->getFunction(Naming::PUBLIC_IN_FUNC);
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

void CTFuzzInstrumentSelf::readInputs(CallInst* TCI, iterator_range<Function::arg_iterator>& args, BoxList& boxes) {
  IRBuilder<> IRB(TCI);

  // create two copies of input variables and read values into them
  for (auto& arg : args)
    boxes.push_back(createArray(IRB, arg.getType()));

  // encoding of pointer arguments
  // the first two bytes are element length (*not byte size*)
  // then read (element size * element length) bytes
  auto GI = [&IRB, &args, &boxes, this](idx_t idx) -> void {
    unsigned i = 0;
    for (auto& arg : args) {
      Type* T = arg.getType();
      IRB.CreateCall(ri->getReadFunc(T), {getElement(IRB, boxes[i], idx)});
      ++i;
    }
  };

  GI(0); GI(1);
  ERASE_TCI()
}

void CTFuzzInstrumentSelf::checkInputs(CallInst* TCI, iterator_range<Function::arg_iterator>& args, const BoxList& boxes, Function* specF) {
  IRBuilder<> IRB(TCI);

  Value* idx = IRB.CreateZExt(TCI->getOperand(0), IntegerType::get(TCI->getContext(), 64));
  unsigned i = 0;
  std::vector<Value*> argVs;

  for (auto& arg : args) {
    argVs.push_back(IRB.CreateLoad(getElement(IRB, boxes[i], idx)));
    ++i;
  }

  IRB.CreateCall(specF, argVs);
  ERASE_TCI()
}

void CTFuzzInstrumentSelf::mergePtrInputs(CallInst* TCI, iterator_range<Function::arg_iterator>& args, const BoxList& boxes, BoxList& ptrBoxes) {
  IRBuilder<> IRB(TCI);
  unsigned i = 0;
  for (auto& arg : args) {
    Type* T = arg.getType();
    if (T->isPointerTy()) {
      AllocaInst* AI = IRB.CreateAlloca(T);
      ptrBoxes.push_back(AI);
      IRB.CreateCall(ri->getMergeFunc(T),
        {AI,
        getElement(IRB, boxes[i], (idx_t)0),
        getElement(IRB, boxes[i], (idx_t)1)});
    }
    ++i;
  }
  ERASE_TCI()
}

// NOTE: we're in a forked process
void CTFuzzInstrumentSelf::execInputFunc(CallInst* TCI, iterator_range<Function::arg_iterator>& args, const BoxList& boxes, const BoxList& ptrBoxes, Function* srcF) {
  IRBuilder<> IRB(TCI);

  Value* idx = IRB.CreateZExt(TCI->getOperand(0), IntegerType::get(TCI->getContext(), 64));
  std::vector<Value*> argVs;

  unsigned i = 0;
  unsigned ptr_i = 0;
  for (auto& arg : args) {
    Type* T = arg.getType();
    auto valueP = boxes[i];
    if (!T->isPointerTy())
      argVs.push_back(IRB.CreateLoad(getElement(IRB, valueP, idx)));
    else {
      // deep copy
      auto PP = ptrBoxes[ptr_i];
      IRB.CreateCall(ri->getCopyFunc(T),
        {PP, getElement(IRB, valueP, idx)});
      argVs.push_back(IRB.CreateLoad(PP));
      ++ptr_i;
    }
    ++i;
  }

  // call the function
  IRB.CreateCall(srcF, argVs);
  ERASE_TCI()
}

Function* getSpecFunction(Module& M) {
  for (auto &F : M.functions()) {
    if (F.hasName() && F.getName().find(Naming::SPEC_FUNC_PREFIX) == 0)
      return &F;
  }
  llvm_unreachable("Unable to find spec function.");
}

void printByte(unsigned char b) {
  char tmp[4];
  sprintf(tmp, "\\x%02X", b);
  std::cout << tmp;
}

void printInt(uint64_t v, unsigned size) {
  unsigned char* p = (unsigned char*)(&v);
  for (unsigned i = 0; i < size; ++i)
    printByte(p[i]);
}

bool flipCoin() {
  return std::rand() & 1U;
}

unsigned short generateLen(Type* ET) {
  if (ET->isStructTy() || ET->isPointerTy())
    return flipCoin()? 1 : 2;
  else {
    bool b1 = flipCoin();
    bool b2 = flipCoin();
    if (b1)
      if (b2)
        return 1;
      else
        return 2;
    else
      if (b2)
        return 3;
      else
        return 4;
  }
}

void CTFuzzInstrumentSelf::generateSeedForT(Type* T) {
  if (!T->isPointerTy())
    if (IntegerType* it = dyn_cast<IntegerType>(T))
      printInt(42, it->getBitWidth() >> 3);
    else if (ArrayType* at = dyn_cast<ArrayType>(T))
      for (unsigned i = 0 ; i < at->getNumElements(); ++i)
        generateSeedForT(at->getElementType());
    else if (StructType* st = dyn_cast<StructType>(T))
      for (unsigned i = 0 ; i < st->getNumElements(); ++i)
        generateSeedForT(st->getElementType(i));
    else
      llvm_unreachable("doesn't support this type");
  else {
    PointerType* pt = cast<PointerType>(T);
    Type* et = pt->getElementType();
    unsigned short len = generateLen(et);
    printInt(len, 2);
    for (unsigned short i = 0; i < len; ++i)
      generateSeedForT(et);
  }
}

void CTFuzzInstrumentSelf::generateSeeds(Function* F) {
  for (auto& arg: F->args())
    generateSeedForT(arg.getType());
}

bool CTFuzzInstrumentSelf::runOnModule(Module& M) {
  Function* srcF = Utils::getFunction(M, Opt::EntryPoint);
  Function* specF = getSpecFunction(M);
  Function* mainF = Utils::getFunction(M, "__ct_fuzz_main");
  ri = new CTFuzzReadInputs(&M);
  this->M = &M;

  if (Opt::SeedNum) {
    std::srand(time(NULL));
    for (unsigned i = 0; i < Opt::SeedNum; ++i) {
      auto s = std::rand();
      std::srand(s);
      generateSeeds(srcF);
      std::srand(s);
      generateSeeds(srcF);
      std::cout << std::endl;
    }
    return true;
  }

  mallocF = Utils::getFunction(M, "__ct_fuzz_malloc_wrapper");

  // this function instruments __ct_fuzz_public_in calls such that
  // the value passed to the these calls are either recorded or checked
  // via calling public_value_handle functions
  insertPublicInHandleFuncs(specF);

  // boxes is a pair of pointers
  // the first element is a pointer to the function argument
  // the second element is a pointer to the element length
  // if the argument is not a pointer, then the second element is nullptr
  BoxList boxes;
  BoxList ptrBoxes;
  auto args = srcF->args();
  readInputs(getCallToReadInputsFunc(mainF), args, boxes);
  checkInputs(getCallToSpecFunc(mainF), args, boxes, specF);
  mergePtrInputs(getCallToMergeFunc(mainF), args, boxes, ptrBoxes);
  execInputFunc(getCallToExecFunc(mainF), args, boxes, ptrBoxes, srcF);

  delete ri;
  return false;
}

// Pass ID variable
char CTFuzzInstrumentSelf::ID = 0;

// Register the pass
static RegisterPass<CTFuzzInstrumentSelf>
X("ct-fuzz-instrument-self", "Instrumentations for constant time fuzzer");
