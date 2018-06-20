#ifndef CT_FUZZ_READ_INPUTS_H
#define CT_FUZZ_READ_INPUTS_H

#include <map>
#include <string>
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

class CTFuzzReadInputs : public llvm::FunctionPass {
  public:
    static char ID;
    CTFuzzReadInputs() : llvm::FunctionPass(ID) {}
    bool runOnFunction(llvm::Function& F) override;
  private:
    std::map<std::string, llvm::Function*> mappings;
    llvm::Function* stdinRF;
    llvm::Function* genericPtrReadF;
    llvm::Function* getReadFunc(llvm::Type* elemT, llvm::Module* M);
};
#endif
