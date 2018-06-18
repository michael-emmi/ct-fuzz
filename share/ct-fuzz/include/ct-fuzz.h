#ifndef CT_FUZZ_H
#define CT_FUZZ_H
#include <stdbool.h>
#include <stdint.h>
#include "ct-fuzz-utils.h"
//#include "ct-fuzz-observation.h"

void PREFIX(public_in)();
void PREFIX(assume)(bool, char* msg);
uint16_t PREFIX(array_len)(void*);

#define CT_FUZZ_SPEC(F) void __ct_fuzz_spec_ ## F
#define CT_FUZZ_ASSUME(cond) PREFIX(assume)(cond, "assumption violation")

#endif
