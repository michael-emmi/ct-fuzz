#ifndef INSTANTIATE_H
#define INSTANTIATE_H

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "read-inputs.h"

#include <vector>
#include <map>
#include <set>
#include <utility>

using namespace llvm;

typedef unsigned char idx_t;
typedef std::vector<AllocaInst*> BoxList;
typedef iterator_range<Function::arg_iterator> argsT;

namespace CTFuzz {
class InstantiateHarness: public ModulePass {
  public:
    static char ID;
    InstantiateHarness() : ModulePass(ID) {}
    virtual bool runOnModule(Module& M);

  private:
    BoxList readInputs(CallInst* TCI, argsT& args,
      std::map<Value*, CallInst*>& funcPtrArgInfo, std::set<Value*>& publicArgs);
    void checkInputs(CallInst* TCI, argsT& args,
      const BoxList& boxes, Function* specF);
    void execInputFunc(CallInst* TCI, argsT& args,
      std::map<Value*, CallInst*>& funcPtrArgInfo, std::set<Value*>& publicArgs,
      BoxList boxes, Function* srcF);
    void analyzeSpecs(Function* specF,
      std::map<Value*, CallInst*>& funcPtrArgInfo, std::set<Value*>& publicArgs);
    std::pair<unsigned, unsigned> getOffset(argsT& args,
      std::map<Value*, CallInst*>& funcPtrArgInfo, std::set<Value*>& publicArgs);
};
}

#endif
