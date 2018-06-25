#ifndef CT_FUZZ_INSTRUMENT_SELF_H
#define CT_FUZZ_INSTRUMENT_SELF_H

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/InstVisitor.h"
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
#include "ct-fuzz-naming.h"
#include "ct-fuzz-options.h"
#include "ct-fuzz-instrument-utils.h"
#include "ct-fuzz-read-inputs.h"

#include <utility>
#include <vector>
#include <map>

typedef unsigned char idx_t;
//typedef std::pair<llvm::AllocaInst*, llvm::AllocaInst*> Boxes;
typedef std::vector<llvm::AllocaInst*> BoxList;

class CTFuzzInstrumentSelf: public llvm::ModulePass {
  public:
    static char ID;
    CTFuzzInstrumentSelf() : llvm::ModulePass(ID) {}
    virtual bool runOnModule(llvm::Module& M);

  private:
    llvm::Function* mallocF;
    void updateMonitors(llvm::Module& M);
    llvm::Function* buildPublicInHandleFunc(llvm::Module* M, llvm::CallInst* CI);
    void insertPublicInHandleFuncs(llvm::Module& M, llvm::Function* specF);
    llvm::CallInst* readInputs(llvm::Module& M, llvm::Function* mainF, llvm::Function* srcF, BoxList& boxes);
    llvm::CallInst* checkInputs(llvm::Module& M, llvm::Function* mainF, llvm::Function* specF, const BoxList& boxes);
    llvm::CallInst* mergePtrInputs(llvm::Module& M, llvm::Function* mainF, llvm::Function* srcF, const BoxList& boxes, BoxList& ptrBoxes);
    llvm::CallInst* execInputFunc(llvm::Module& M, llvm::Function* mainF, llvm::Function* srcF, const BoxList& boxes, const BoxList& ptrBoxes);
    void generateSeeds(llvm::Function* F);
    void generateSeedForT(llvm::Type* T);
    CTFuzzReadInputs* ri;
};

#endif
