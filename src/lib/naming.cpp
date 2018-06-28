#include "naming.h"

namespace CTFuzz {

const std::string Naming::PUBLIC_IN_FUNC = "__ct_fuzz_public_in";
const std::string Naming::PUBLIC_IN_HANDLE_FUNC = "__ct_fuzz_handle_public_value";
const std::string Naming::SPEC_FUNC_PREFIX = "__ct_fuzz_spec";

bool Naming::isCTFuzzFunc(std::string name) {
  return name.find("__ct_fuzz") == 0;
}
}
