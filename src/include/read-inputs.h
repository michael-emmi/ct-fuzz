#ifndef READ_INPUTS_H
#define READ_INPUTS_H

#include <map>
#include <string>
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Module.h"

namespace CTFuzz {

using namespace llvm;

class ReadInputs {
  public:
    ReadInputs(Module* M);
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
