#ifndef READ_INPUTS_H
#define READ_INPUTS_H

#include <map>
#include <string>
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Module.h"
#include "instrument-utils.h"

namespace CTFuzz {

using namespace llvm;
typedef InstrumentUtils Utils;

class ReadInputs {
  public:
    ReadInputs(Module* M) : M(M) {
      stdinRF = Utils::getFunction(*M, "__ct_fuzz_stdin_read");
      genericPtrReadF = Utils::getFunction(*M, "__ct_fuzz_read_ptr_generic");
      genericPtrMergeF = Utils::getFunction(*M, "__ct_fuzz_merge_ptr_generic");
      genericPtrCopyF = Utils::getFunction(*M, "__ct_fuzz_deep_copy_ptr_generic");
      memcpyF = Utils::getFunction(*M, "__ct_fuzz_memcpy_wrapper");
    }

    Function* getReadFunc(Type* elemT);
    Function* getMergeFunc(Type* elemT);
    Function* getCopyFunc(Type* elemT);
  private:
    Module* M;
    std::map<std::string, Function*> readf_mappings;
    std::map<std::string, Function*> mergef_mappings;
    std::map<std::string, Function*> copyf_mappings;
    unsigned getIndirectionLevel(Type* T);
    Type* getUltimateElemTy(Type* T);
    std::string getTypeName(Type* T);
    Function* stdinRF;
    Function* genericPtrReadF;
    Function* genericPtrMergeF;
    Function* genericPtrCopyF;
    Function* memcpyF;
};
}
#endif
