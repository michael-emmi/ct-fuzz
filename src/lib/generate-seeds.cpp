#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "generate-seeds.h"

#include <iomanip>
#include <iostream>
#include <fstream>
#include <vector>
#include <utility>

namespace {
typedef std::vector<CallInst*> Invocations;
typedef std::pair<uint64_t, unsigned> ConstIntInfo;

const cl::opt<std::string> EntryPoint(
  "entry-point",
  cl::desc("Entry point function name")
);

const cl::opt<unsigned> SeedNum(
  "seed-num",
  cl::desc("The number of seeds automatically generated per invocation"),
  cl::init(0)
);

const cl::opt<std::string> OutputFile(
  "output-file",
  cl::desc("The output file that contains the seeds")
);


ConstIntInfo getConstantIntInfo(ConstantInt* CI) {
  return std::make_pair(CI->getZExtValue(), CI->getBitWidth() >> 3);
}

ConstantInt* getConstantInt(Value* V) {
  if (ConstantInt* CI = dyn_cast<ConstantInt>(V))
    return CI;
  if (LoadInst* LI = dyn_cast<LoadInst>(V))
    return getConstantInt(LI->getPointerOperand());
  return nullptr;
}

void printByte(unsigned char b, std::ostream& ss) {
  char tmp[4];
  sprintf(tmp, "\\x%02X", b);
  ss << tmp;
}

void printInt(uint64_t v, unsigned size, std::ostream& ss) {
  unsigned char* p = (unsigned char*)(&v);
  for (unsigned i = 0; i < size; ++i)
    printByte(p[i], ss);
}

Value* stripBitCasts(Value* V) {
  if (auto bi = dyn_cast<BitCastInst>(V))
    return stripBitCasts(bi->getOperand(0));
  else if (auto be = dyn_cast<ConstantExpr>(V))
    if (be->isCast())
      return stripBitCasts(be->getAsInstruction());
  return V;
}

Constant* getElement(Value* V) {
  V = stripBitCasts(V);
  if (isa<GlobalVariable>(V))
      return cast<Constant>(V);
  else if (isa<ConstantExpr>(V) &&
      cast<ConstantExpr>(V)->isGEPWithNoNotionalOverIndexing()) {
    Constant* CT = cast<Constant>(V);
    GlobalVariable* G = cast<GlobalVariable>(CT->getOperand(0));
    assert(G->hasInitializer && "current assumption, no?");
    Constant* C = G->getInitializer();
    for (unsigned i = 2; i < CT->getNumOperands() - 1; ++i) {
      auto idx = getConstantIntInfo(cast<ConstantInt>(CT->getOperand(i))).first;
      C = C->getAggregateElement(idx);
    }
    return C;
  } else
    llvm_unreachable("dude, who are you?");
}

Invocations getInvocations(Function* seedF) {
  Module* M = seedF->getParent();
  Function* srcF = M->getFunction(EntryPoint);
  Invocations ret;
  for (auto U: srcF->users())
    if (isa<CallInst>(U) && cast<Instruction>(U)->getFunction() == seedF)
      ret.push_back(cast<CallInst>(U));
  return ret;
}
}

namespace CTFuzz {
void GenerateSeeds::generateSeedForVT(Value* V, Type* T, std::ostream& ss) {
  //errs() << "V: " << *V << "\n";
  //errs() << "T: " << *T << "\n";
  if (!T->isPointerTy())
    if (IntegerType* it = dyn_cast<IntegerType>(T)) {
      auto CI = getConstantInt(V);
      if (CI) {
        auto CIIF = getConstantIntInfo(CI);
        printInt(CIIF.first, CIIF.second, ss);
      } else
        printInt(42, it->getBitWidth() >> 3, ss);
    }
    else if (ArrayType* at = dyn_cast<ArrayType>(T)) {
      assert(isa<ConstantArray>(V) && "shouldn't V be a constant array?");
      auto CA = cast<ConstantArray>(V);
      for (unsigned i = 0 ; i < at->getNumElements(); ++i)
        generateSeedForVT(CA->getAggregateElement(i), at->getElementType(), ss);
    }
    else if (StructType* st = dyn_cast<StructType>(T)) {
      assert(isa<ConstantStruct>(V) && "shouldn't V be a constant struct?");
      auto CS = cast<ConstantStruct>(V);
      for (unsigned i = 0 ; i < st->getNumElements(); ++i)
        generateSeedForVT(CS->getAggregateElement(i), st->getElementType(i), ss);
    }
    else
      llvm_unreachable("doesn't support this type");
  else {
    PointerType* pt = cast<PointerType>(T);
    Type* et = pt->getElementType();
    Constant* C = getElement(V);
    C->getType()->print(errs());
    unsigned len = C->getType()->isArrayTy() ?
      cast<ArrayType>(C->getType())->getNumElements() : 1;
    printInt(len, 2, ss);
    errs() << "len: " << len << "\n";
      errs() << "C2: " << *C << "\n";
    errs() << *C->getAggregateElement(0U) << "\n";
    for (unsigned i = 0; i < len; ++i) {
      errs() << "i: " << i << *C->getAggregateElement(i) << "\n";
      generateSeedForVT(C->getAggregateElement(i), et, ss);
    }
  }
}

void GenerateSeeds::generateSeedForInvocation(CallInst* CI, std::ostream& ss) {
  Function* F = CI->getCalledFunction();
  unsigned i = 0;
  for (auto& arg: F->args()) {
    generateSeedForVT(CI->getArgOperand(i), arg.getType(), ss);
    ++i;
  }
}

bool GenerateSeeds::runOnFunction(Function& F) {
  if (F.hasName() && F.getName().str() == "__ct_fuzz_seed_" + EntryPoint) {
    std::ofstream f;
    if (OutputFile.getNumOccurrences())
      f.open(OutputFile);

    std::ostream& ss = OutputFile.getNumOccurrences()? f : std::cout;

    auto Is = getInvocations(&F);
    for (auto I : Is) {
      generateSeedForInvocation(I, ss);
      ss << "\n";
    }

    if (OutputFile.getNumOccurrences())
      static_cast<std::ofstream*>(&ss)->close();
  }

  return true;
}

// Pass ID variable
char GenerateSeeds::ID = 0;

// Register the pass
static RegisterPass<GenerateSeeds>
X("ct-fuzz-generate-seeds", "Seed generation for constant time fuzzer");

}
