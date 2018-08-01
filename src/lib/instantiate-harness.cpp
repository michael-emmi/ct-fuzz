#include "instantiate-harness.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>

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
const std::string POINTER_LENGTH_FUNC = "__ct_fuzz_ptr_len";
const std::string STDIN_READ_FUNC = "__ct_fuzz_stdin_read";
const std::string MEMCPY_WRAPPER = "__ct_fuzz_memcpy_wrapper";

CallInst* getCallToReadInputsFunc(Module& M) {
  return getCallToFuncOnce(getFunction(M, READ_INPUT_FUNC));
}

CallInst* getCallToSpecFunc(Module& M) {
  return getCallToFuncOnce(getFunction(M, SPEC_FUNC_PREFIX));
}

CallInst* getCallToExecFunc(Module& M) {
  return getCallToFuncOnce(getFunction(M, EXEC_FUNC));
}

std::vector<CallInst*> getPublicInCalls(Module& M) {
  return getCallToFunc(M.getFunction(PUBLIC_IN_FUNC));
}

std::vector<CallInst*> getPointerLengthCalls(Module& M) {
  return getCallToFunc(M.getFunction(POINTER_LENGTH_FUNC));
}

}

namespace CTFuzz {
const cl::opt<std::string> EntryPoint(
  "entry-point",
  cl::desc("Entry point function name")
);

inline AllocaInst* createArray(IRBuilder<>& IRB, Type* T) {
  T = T->isPointerTy()? T : PointerType::getUnqual(T);
  return IRB.CreateAlloca(ArrayType::get(T, 2));
}

inline ConstantInt* createIdx(Value* AI, idx_t idx) {
  return ConstantInt::get(IntegerType::get(AI->getContext(), 64), idx);
}

inline Value* getElement(IRBuilder<>& IRB, AllocaInst* AI, idx_t idx) {
  return IRB.CreateGEP(AI, {createIdx(AI, 0), createIdx(AI, idx)});
}

inline Value* createGEP(IRBuilder<>& IRB, Value* base, unsigned offset) {
  return IRB.CreateGEP(base, {createIdx(base, offset)});
}

inline Value* getElement(IRBuilder<>& IRB, AllocaInst* AI, Value* idx) {
  return IRB.CreateGEP(AI, {createIdx(AI, 0), idx});
}

BoxList InstantiateHarness::readInputs(CallInst* TCI, argsT& args,
  LengthBindings& LB, std::set<Value*>& publicArgs) {
  IRBuilder<> IRB(TCI);
  Module* M = TCI->getModule();
  auto DL = M->getDataLayout();
  Function* mallocF = getFunction(*M, MALLOC_WRAPPER);
  Function* stdinRF = getFunction(*M, STDIN_READ_FUNC);

  BoxList ret;

  auto p = getOffset(args, LB, publicArgs);
  unsigned first = p.first;
  unsigned all = p.second;

  // create a space and read from stdin
  for (auto& arg : args)
    ret.push_back(createArray(IRB, arg.getType()));

  Constant* Size = ConstantInt::get(getFirstArg(mallocF)->getType(), all);
  Value* mallocC = IRB.CreateCall(mallocF, {Size});
  IRB.CreateCall(stdinRF, {mallocC, Size});

  mallocC = IRB.CreateBitCast(mallocC, Type::getInt8PtrTy(M->getContext()));
  unsigned firstOffset = 0;
  unsigned secondOffset = first;
  unsigned i = 0;
  // create two copies of input variables and `read` values into them
  for (auto& arg : args) {
    Value* arg1_ptr = nullptr;
    Value* arg2_ptr = nullptr;
    Value* gep1 = getElement(IRB, ret[i], (idx_t)0);
    Value* gep2 = getElement(IRB, ret[i], 1);

    Type* T = arg.getType();
    bool isPointer = T->isPointerTy();
    PointerType* PT = isPointer? cast<PointerType>(T) : PointerType::getUnqual(T);

    bool isPublic = publicArgs.count(&arg) > 0;
    unsigned tSize = isPointer?
      LB.getSizeInBytes(&arg) : getTypeSize(T);

    if (isPublic)
      arg2_ptr = arg1_ptr = IRB.CreateBitCast(createGEP(IRB, mallocC, firstOffset), PT);
    else {
      arg1_ptr = IRB.CreateBitCast(createGEP(IRB, mallocC, firstOffset), PT);
      arg2_ptr = IRB.CreateBitCast(createGEP(IRB, mallocC, secondOffset), PT);
      secondOffset += tSize;
    }

    firstOffset += tSize;

    IRB.CreateStore(arg1_ptr, gep1);
    IRB.CreateStore(arg2_ptr, gep2);
    ++i;
  }

  ERASE_TCI()
  return ret;
}

void InstantiateHarness::checkInputs(CallInst* TCI, argsT& args,
  const BoxList& boxes, Function* specF) {
  IRBuilder<> IRB(TCI);

  Value* idx = IRB.CreateZExt(TCI->getOperand(0), IntegerType::get(TCI->getContext(), 64));
  std::vector<Value*> argVs;
  unsigned i = 0;

  for (auto& arg : args) {
    if (arg.getType()->isPointerTy())
      argVs.push_back(IRB.CreateLoad(getElement(IRB, boxes[i], idx)));
    else
      argVs.push_back(IRB.CreateLoad(IRB.CreateLoad(getElement(IRB, boxes[i], idx))));
    ++i;
  }

  IRB.CreateCall(specF, argVs);
  ERASE_TCI()
}

// NOTE: we're in a forked process
void InstantiateHarness::execInputFunc(CallInst* TCI, argsT& args,
  LengthBindings& LB, std::set<Value*>& publicArgs, BoxList boxes, Function* srcF) {
  IRBuilder<> IRB(TCI);
  Module* M = TCI->getModule();
  Function* mallocF = getFunction(*M, MALLOC_WRAPPER);
  Function* memcpyF = getFunction(*M, MEMCPY_WRAPPER);

  Value* idx = IRB.CreateZExt(TCI->getOperand(0), IntegerType::get(TCI->getContext(), 64));
  std::vector<Value*> argVs;

  unsigned i = 0;

  for (auto& arg : args) {
    Type* T = arg.getType();
    auto valueP = boxes[i];
    if (!T->isPointerTy()) {
      if (LB.isLenArg(&arg)) {
        unsigned moduloSize = LB.getModuloLen(&arg);
        argVs.push_back(IRB.CreateURem(
          IRB.CreateLoad(
            IRB.CreateLoad(getElement(IRB, valueP, idx))),
          ConstantInt::get(T, moduloSize)));
      } else
        argVs.push_back(IRB.CreateLoad(
          IRB.CreateLoad(getElement(IRB, valueP, idx))));

    } else {
      bool isPublic = publicArgs.count(&arg) > 0;
      if (!isPublic) {
        // we have to copy the two arrays here otherwise we would
        // end up having two arrays having distinct addresses
        Constant* Size = ConstantInt::get(getFirstArg(mallocF)->getType(),
          LB.getSizeInBytes(&arg));
        Value* mallocC = IRB.CreateCall(mallocF, {Size});
        IRB.CreateCall(memcpyF,
          {IRB.CreateBitCast(mallocC, getFirstArg(memcpyF)->getType()),
          IRB.CreateBitCast(IRB.CreateLoad(getElement(IRB, valueP, idx)), getSecondArg(memcpyF)->getType()),
          Size});
        argVs.push_back(IRB.CreateBitCast(mallocC, T));
      } else
        argVs.push_back(IRB.CreateLoad(getElement(IRB, valueP, idx)));
    }
    ++i;
  }

  // call the function
  IRB.CreateCall(srcF, argVs);
  ERASE_TCI()
}

std::pair<unsigned, unsigned> InstantiateHarness::getOffset(argsT& args,
  LengthBindings& LB, std::set<Value*>& publicArgs) {
  unsigned first = 0;
  unsigned second = 0;
  for (auto& arg : args) {
    unsigned argOffset = 0;

    if (auto pt = dyn_cast<PointerType>(arg.getType()))
      argOffset = LB.getSizeInBytes(&arg);
    else {
      assert(arg.getType()->isIntegerTy() && "floating-point types in crypto code?");
      argOffset = getTypeSize(arg.getType());
    }

    first += argOffset;
    if (!publicArgs.count(&arg))
      second += argOffset;
  }
  return std::make_pair(first, first+second);
}

void InstantiateHarness::analyzeSpecs(Function* specF, LengthBindings& LB, std::set<Value*>& publicArgs) {
  std::vector<CallInst*> ptrLenCalls = getPointerLengthCalls(*specF->getParent());
  std::vector<CallInst*> publicInCalls = getPublicInCalls(*specF->getParent());
  std::set<Value*> funcPtrArgs;
  std::set<Value*> funcArgs;

  for (auto& arg : specF->args()) {
    if (arg.getType()->isPointerTy())
      funcPtrArgs.insert(&arg);
    funcArgs.insert(&arg);
  }

  for (auto ptrLenCall : ptrLenCalls) {
    Value* arg = getFuncArgFromCallArg(ptrLenCall->getArgOperand(0));
    assert(funcPtrArgs.count(arg) == 1 && "constrained value is not a pointer argument");
    LB.addBinding(ptrLenCall);
  }

  for (auto publicInCall : publicInCalls) {
    Value* arg = getFuncArgFromCallArg(publicInCall->getArgOperand(0));
    assert(funcArgs.count(arg) == 1 && "public in value is not an argument");
    publicArgs.insert(arg);
  }

  /*
  auto args = specF->args();
  auto p = getOffset(args, funcPtrArgInfo, publicArgs);

  errs() << "half offset: " << p.first;
  errs() << ", all offset: " << p.second;
  errs() << "\n";

  for (auto& info : funcPtrArgInfo) {
    Value* arg = info.first;
    CallInst* ci = info.second;
    errs() << "arg: " << *arg;
    errs() << " info: " << *ci;
    errs() << "\n";
  }

  //for (auto e : ptrLenCalls)
  //  e->eraseFromParent();
  */
  for (auto e : publicInCalls)
    e->eraseFromParent();
}

bool InstantiateHarness::runOnModule(Module& M) {
  Function* srcF = getFunction(M, EntryPoint);
  Function* specF = getFunction(M, SPEC_FUNC_PREFIX+"_"+EntryPoint);
  std::set<Value*> publicArgs;
  LengthBindings LB;

  analyzeSpecs(specF, LB, publicArgs);

  auto args = specF->args();
  auto boxes = readInputs(getCallToReadInputsFunc(M), args, LB, publicArgs);
  checkInputs(getCallToSpecFunc(M), args, boxes, specF);
  execInputFunc(getCallToExecFunc(M), args, LB, publicArgs, boxes, srcF);

  // clean up
  //for (auto& t : funcPtrArgInfo)
  //  t.second->eraseFromParent();
  LB.clean_up();

  return false;
}

// Pass ID variable
char InstantiateHarness::ID = 0;

// Register the pass
static RegisterPass<InstantiateHarness>
X("ct-fuzz-instantiate-harness", "Harness instantiation for constant time fuzzer");
}
