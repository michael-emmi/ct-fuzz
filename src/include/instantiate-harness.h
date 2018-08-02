#ifndef INSTANTIATE_H
#define INSTANTIATE_H

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
//#include "read-inputs.h"
#include "utils.h"

#include <vector>
#include <map>
#include <set>
#include <utility>

using namespace llvm;

typedef std::vector<AllocaInst*> BoxList;
typedef iterator_range<Function::arg_iterator> argsT;

namespace CTFuzz {
struct LengthBindings {
  struct LengthBinding {
    Value* ptr;
    Value* len;
    Value* maxLen;
    CallInst* CI;

    LengthBinding(CallInst* CI) {
      ptr = getFuncArgFromCallArg(CI->getArgOperand(0));
      len = CI->getArgOperand(1);
      maxLen = CI->getArgOperand(2);
      this->CI = CI;
    }

    //~LengthBinding() {
    //  CI->eraseFromParent();
    //}

    unsigned getFixedLen(void) {
      ConstantInt* cLen = dyn_cast<ConstantInt>(maxLen);

      if (auto tc = dyn_cast<ConstantInt>(len))
        if(tc->getZExtValue())
          cLen = tc;

      assert(cLen && "not able to get a constant pointer length");
      return cLen->getZExtValue();
    }

    unsigned getFixedSizeInBytes(void) {
      return getFixedLen()*getTypeSize(
        cast<PointerType>(ptr->getType())->getElementType());
    }

    bool hasAssociatingLenArg(void) {
      return !isa<ConstantInt>(len);
    }

    Value* getAssociatingLenArg(void) {
      return getFuncArgFromCallArg(len);
    }
  };

  std::vector<LengthBinding> bindings;

  void addBinding(CallInst* CI) {
    bindings.push_back(LengthBinding(CI));
  }

  bool isLenArg(Value* V) {
    for (auto& b : bindings)
      if (b.hasAssociatingLenArg() && b.getAssociatingLenArg() == V)
        return true;
    return false;
  }

  unsigned getModuloLen(Value* lenArg) {
    for (auto & b : bindings)
      if (b.hasAssociatingLenArg() && b.getAssociatingLenArg() == lenArg)
        return b.getFixedLen();
    llvm_unreachable("should be a length argument");
  }

  unsigned getSizeInBytes(Value* ptrArg) {
    for (auto & b : bindings)
      if (b.ptr == ptrArg)
        return b.getFixedSizeInBytes();
    llvm_unreachable("not a pointer argument");
  }

  void clean_up() {
    for (auto& b : bindings)
      b.CI->eraseFromParent();
  }
};

class InstantiateHarness: public ModulePass {
  public:
    static char ID;
    InstantiateHarness() : ModulePass(ID) {}
    virtual bool runOnModule(Module& M);

  private:
    BoxList readInputs(CallInst* TCI, argsT& args,
      LengthBindings& LB, std::set<Value*>& publicArgs);
    void checkInputs(CallInst* TCI, argsT& args,
      LengthBindings& LB, const BoxList& boxes, Function* specF);
    void execInputFunc(CallInst* TCI, argsT& args,
      LengthBindings& LB, std::set<Value*>& publicArgs,
      BoxList boxes, Function* srcF);
    void analyzeSpecs(Function* specF,
      LengthBindings& LB, std::set<Value*>& publicArgs);
    std::pair<unsigned, unsigned> getOffset(argsT& args,
      LengthBindings& LB, std::set<Value*>& publicArgs);
};
}

#endif
