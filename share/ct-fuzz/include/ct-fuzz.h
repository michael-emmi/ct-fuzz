#ifndef CT_FUZZ_H
#define CT_FUZZ_H
#include <stdbool.h>
#include <stdint.h>
#include "ct-fuzz-utils.h"

#ifdef __cplusplus
extern "C" {
#endif

void NS(public_in)();
void NS(assume)(bool, char* msg);
uint16_t NS(get_arr_len)(const char* const);
void NS(ptr_len)(unsigned char* ptr, unsigned short len, unsigned short max_len);

#ifdef __cplusplus
}
#endif


#define CT_FUZZ_SPEC(RET,F,...) \
  RET F(__VA_ARGS__); \
  void* __ct_fuzz_dummy_ ## F() { return (void*)&F;} \
  void __ct_fuzz_spec_ ## F(__VA_ARGS__)

#define CT_FUZZ_SEED(RET,F,...) \
  RET F(__VA_ARGS__); \
  void __ct_fuzz_invocation_ ## F(__VA_ARGS__); \
  void __ct_fuzz_seed_ ## F(void)

#define SEED_UNIT(T,N,...) \
  static const T N = __VA_ARGS__;

#define SEED_1D_ARR(T,N,L,...) \
  static const T N[L] = __VA_ARGS__;

#define SEED_2D_ARR(T,N,L1,L2,...) \
  static const T N[L1][L2] = __VA_ARGS__;

#define SEED_3D_ARR(T,N,L1,L2,L3,...) \
  static const T N[L1][L2][L3] = __VA_ARGS__;

#define PRODUCE(F,...) \
  __ct_fuzz_invocation_ ## F(__VA_ARGS__);

#define CT_FUZZ_ASSUME(cond) NS(assume)(cond, "assumption violation")

#endif
