#ifndef CT_FUZZ_H
#define CT_FUZZ_H
#include "ct-fuzz-observation.h"

void PREFIX(public_in)();

#define CT_FUZZ_SPEC(F) void __ct_fuzz_spec_ ## F

#endif
