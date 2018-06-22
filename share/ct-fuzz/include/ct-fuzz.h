#ifndef CT_FUZZ_H
#define CT_FUZZ_H
#include <stdbool.h>
#include <stdint.h>
#include "ct-fuzz-utils.h"

void PREFIX(public_in)();
void PREFIX(assume)(bool, char* msg);
uint16_t PREFIX(get_arr_len)(const char* const);

#define CT_FUZZ_SPEC(RET,F,...) \
  RET F(__VA_ARGS__); \
  void* __ct_fuzz_dummy_ ## F() { return (void*)&F;} \
  void __ct_fuzz_spec_ ## F(__VA_ARGS__)

#define CT_FUZZ_ASSUME(cond) PREFIX(assume)(cond, "assumption violation")

#endif
