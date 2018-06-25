#ifndef CT_FUZZ_READ_INPUTS_H
#define CT_FUZZ_READ_INPUTS_H

#include <map>
#include <string>
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "ct-fuzz-instrument-utils.h"

typedef CTFuzzInstrumentUtils Utils;
//class CTFuzzReadInputs : public llvm::FunctionPass {
class CTFuzzReadInputs {
  public:
    /*
    static char ID;
    CTFuzzReadInputs() : llvm::FunctionPass(ID) {}
    bool runOnFunction(llvm::Function& F) override;
    */
    CTFuzzReadInputs(llvm::Module* M) {
      stdinRF = Utils::getFunction(*M, "__ct_fuzz_stdin_read");
      genericPtrReadF = Utils::getFunction(*M, "__ct_fuzz_read_ptr_generic");
      stdoutWF = Utils::getFunction(*M, "__ct_fuzz_stdout_write");
      genericPtrMergeF = Utils::getFunction(*M, "__ct_fuzz_merge_ptr_generic");
      genericPtrCopyF = Utils::getFunction(*M, "__ct_fuzz_deep_copy_ptr_generic");
      memcpyF = Utils::getFunction(*M, "__ct_fuzz_memcpy_wrapper");
    }

    llvm::Function* getReadFunc(llvm::Type* elemT, llvm::Module* M);
    llvm::Function* getMergeFunc(llvm::Type* elemT, llvm::Module* M);
    llvm::Function* getCopyFunc(llvm::Type* elemT, llvm::Module* M);
  private:
    std::map<std::string, llvm::Function*> readf_mappings;
    std::map<std::string, llvm::Function*> mergef_mappings;
    std::map<std::string, llvm::Function*> copyf_mappings;
    unsigned getIndirectionLevel(llvm::Type* T);
    llvm::Type* getUltimateElemTy(llvm::Type* T);
    std::string getTypeName(llvm::Type* T);
    llvm::Function* stdinRF;
    llvm::Function* stdoutWF;
    llvm::Function* genericPtrReadF;
    llvm::Function* genericPtrMergeF;
    llvm::Function* genericPtrCopyF;
    llvm::Function* memcpyF;
};
#endif
