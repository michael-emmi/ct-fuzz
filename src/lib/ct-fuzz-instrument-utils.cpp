#include "ct-fuzz-instrument-utils.h"
#include <vector>

using namespace llvm;

Function* CTFuzzInstrumentUtils::getFunction(Module& M, std::string name) {
  Function* f = M.getFunction(name);
  assert(f && "Unable to get function in this module.");
  return f;
}

Value* CTFuzzInstrumentUtils::getSecondArg(Function* F) {
  auto I = F->arg_begin();
  return &*(++I);
}

Value* CTFuzzInstrumentUtils::getFirstArg(Function* F) {
  return &*(F->arg_begin());
}

std::vector<CallInst*> CTFuzzInstrumentUtils::getCallFromFunc(Function* F, std::string FN) {
  std::vector<CallInst*> cis;
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    if (CallInst* ci = dyn_cast<CallInst>(&*I)) {
      Function* f = ci->getCalledFunction();
      if (!f)
        f = dyn_cast<Function>(ci->getCalledValue()->stripPointerCasts());
      if (f && f->hasName()) {
        if (f->getName().str() == FN) {
          assert((ci->getNumArgOperands() == 1 || ci->getNumArgOperands() == 2) && "Assume only one or two arguments to public_in calls");
          cis.push_back(ci);
        }
      }
    }
  }
  return cis;
}

CallInst* CTFuzzInstrumentUtils::getCallToFuncOnce(Function* F, std::string FN) {
  auto ret = getCallFromFunc(F, FN);
  assert(ret.size() == 1 && "Should see just one call site.");
  return ret[0];
}
