#include "instantiate-harness.h"
#include "utils.h"
#include "llvm/Support/CommandLine.h"

#define ERASE_TCI() TCI->eraseFromParent();

using namespace llvm;

namespace {
using namespace CTFuzz;

const std::string PUBLIC_IN_FUNC = "__ct_fuzz_public_in";
const std::string PUBLIC_IN_HANDLE_FUNC = "__ct_fuzz_handle_public_value";
const std::string SPEC_FUNC_PREFIX = "__ct_fuzz_spec";
const std::string READ_INPUT_FUNC = "__ct_fuzz_read_inputs";
const std::string EXEC_FUNC = "__ct_fuzz_exec";
const std::string MERGE_FUNC = "__ct_fuzz_merge_ptr_inputs";
const std::string MALLOC_WRAPPER = "__ct_fuzz_malloc_wrapper";

CallInst* getCallToReadInputsFunc(Module& M) {
  return getCallToFuncOnce(getFunction(M, READ_INPUT_FUNC));
}

CallInst* getCallToSpecFunc(Module& M) {
  return getCallToFuncOnce(getFunction(M, SPEC_FUNC_PREFIX));
}

CallInst* getCallToExecFunc(Module& M) {
  return getCallToFuncOnce(getFunction(M, EXEC_FUNC));
}

CallInst* getCallToMergeFunc(Module& M) {
  return getCallToFuncOnce(getFunction(M, MERGE_FUNC));
}

std::vector<CallInst*> getPublicInCalls(Module& M) {
  return getCallToFunc(M.getFunction(PUBLIC_IN_FUNC));
}
}

namespace CTFuzz {
const cl::opt<std::string> EntryPoint(
  "entry-point",
  cl::desc("Entry point function name")
);

Function* InstantiateHarness::buildPublicInHandleFunc(CallInst* CI) {
  static unsigned counter = 0;
  Module* M = CI->getModule();
  auto DL = M->getDataLayout();
  Type* sizeT = getFirstArg(
    getFunction(*M, MALLOC_WRAPPER))->getType();
  Type* T = CI->getArgOperand(0)->getType();
  std::vector<Type*> ts;

  Function* CF = getFunction(*M, PUBLIC_IN_HANDLE_FUNC);

  for(unsigned i = 0; i < CI->getNumArgOperands(); ++i)
    ts.push_back(CI->getArgOperand(i)->getType());

  FunctionType* newFT = FunctionType::get(Type::getVoidTy(M->getContext()), ts, false);
  std::string newFN = PUBLIC_IN_FUNC + "_" + std::to_string(counter);
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
    IRB.CreateStore(getFirstArg(newF), P);
    // FIXME: I'm not sure if GetTypeSizeInBits is proper here
    uint64_t size = DL.getTypeSizeInBits(T) >> 3;
    S = ConstantInt::get(getSecondArg(CF)->getType(), size);
  } else {
    P = getFirstArg(newF);
    // note that the second argument for __ct_fuzz_public_in function is
    // the number of elements in this array
    S = getByteSizeInSizeT(IRB, DL,
      getSecondArg(newF), cast<PointerType>(T)->getElementType(), sizeT);
  }

  IRB.CreateCall(CF, {IRB.CreateBitCast(P, CF->arg_begin()->getType()), S});

  IRB.CreateRetVoid();

  counter++;
  return newF;
}

void InstantiateHarness::insertPublicInHandleFuncs(Module& M) {
  auto cis = getPublicInCalls(M);
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

  auto PF = M.getFunction(PUBLIC_IN_FUNC);
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

void InstantiateHarness::readInputs(CallInst* TCI,
  argsT& args, BoxList& boxes, ReadInputs& ri) {
  IRBuilder<> IRB(TCI);

  // create two copies of input variables and read values into them
  for (auto& arg : args)
    boxes.push_back(createArray(IRB, arg.getType()));

  // encoding of pointer arguments
  // the first two bytes are element length (*not byte size*)
  // then read (element size * element length) bytes
  auto GI = [&IRB, &args, &boxes, &ri](idx_t idx) -> void {
    unsigned i = 0;
    for (auto& arg : args) {
      Type* T = arg.getType();
      IRB.CreateCall(ri.getReadFunc(T), {getElement(IRB, boxes[i], idx)});
      ++i;
    }
  };

  GI(0); GI(1);
  ERASE_TCI()
}

void InstantiateHarness::checkInputs(CallInst* TCI,
  const BoxList& boxes, Function* specF) {
  IRBuilder<> IRB(TCI);

  Value* idx = IRB.CreateZExt(TCI->getOperand(0), IntegerType::get(TCI->getContext(), 64));
  std::vector<Value*> argVs;

  for (unsigned i = 0; i < specF->arg_size(); ++i)
    argVs.push_back(IRB.CreateLoad(getElement(IRB, boxes[i], idx)));

  IRB.CreateCall(specF, argVs);
  ERASE_TCI()
}

void InstantiateHarness::mergePtrInputs(CallInst* TCI,
  argsT& args, const BoxList& boxes, BoxList& ptrBoxes, ReadInputs& ri) {
  IRBuilder<> IRB(TCI);
  unsigned i = 0;
  for (auto& arg : args) {
    Type* T = arg.getType();
    if (T->isPointerTy()) {
      AllocaInst* AI = IRB.CreateAlloca(T);
      ptrBoxes.push_back(AI);
      IRB.CreateCall(ri.getMergeFunc(T),
        {AI,
        getElement(IRB, boxes[i], (idx_t)0),
        getElement(IRB, boxes[i], (idx_t)1)});
    }
    ++i;
  }
  ERASE_TCI()
}

// NOTE: we're in a forked process
void InstantiateHarness::execInputFunc(CallInst* TCI,
  argsT& args, const BoxList& boxes, const BoxList& ptrBoxes,
  Function* srcF, ReadInputs& ri) {
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
      IRB.CreateCall(ri.getCopyFunc(T),
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

bool InstantiateHarness::runOnModule(Module& M) {
  Function* srcF = getFunction(M, EntryPoint);
  Function* specF = getFunction(M, SPEC_FUNC_PREFIX+"_"+EntryPoint);
  ReadInputs ri(&M);

  // this function instruments __ct_fuzz_public_in calls such that
  // the value passed to the these calls are either recorded or checked
  // via calling public_value_handle functions
  insertPublicInHandleFuncs(M);

  // boxes is a pair of pointers
  // the first element is a pointer to the function argument
  // the second element is a pointer to the element length
  // if the argument is not a pointer, then the second element is nullptr
  BoxList boxes;
  BoxList ptrBoxes;
  auto args = srcF->args();
  readInputs(getCallToReadInputsFunc(M),
    args, boxes, ri);
  checkInputs(getCallToSpecFunc(M), boxes, specF);
  mergePtrInputs(getCallToMergeFunc(M),
    args, boxes, ptrBoxes, ri);
  execInputFunc(getCallToExecFunc(M),
    args, boxes, ptrBoxes, srcF, ri);

  return false;
}

// Pass ID variable
char InstantiateHarness::ID = 0;

// Register the pass
static RegisterPass<InstantiateHarness>
X("ct-fuzz-instantiate-harness", "Harness instantiation for constant time fuzzer");
}
