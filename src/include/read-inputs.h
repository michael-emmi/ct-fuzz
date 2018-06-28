#ifndef READ_INPUTS_H
#define READ_INPUTS_H

#include <map>
#include <string>
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Module.h"
#include "instrument-utils.h"

#define STDIN_READ_FUNC "__ct_fuzz_stdin_read"
#define READ_PTR_GENERIC "__ct_fuzz_read_ptr_generic"
#define MERGE_PTR_GENERIC "__ct_fuzz_merge_ptr_generic"
#define COPY_PTR_GENERIC "__ct_fuzz_deep_copy_ptr_generic"
#define MEMCPY_WRAPPER "__ct_fuzz_memcpy_wrapper"

namespace CTFuzz {

using namespace llvm;

class ReadInputs {
  public:
    ReadInputs(Module* M) : M(M) {
      stdinRF = getFunction(*M, STDIN_READ_FUNC);
      genericPtrReadF = getFunction(*M, READ_PTR_GENERIC);
      genericPtrMergeF = getFunction(*M, MERGE_PTR_GENERIC);
      genericPtrCopyF = getFunction(*M, COPY_PTR_GENERIC);
      memcpyF = getFunction(*M, MEMCPY_WRAPPER);
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
