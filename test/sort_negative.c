#include <stdio.h>
#include "ct-fuzz.h"

void sort2(int *out2, int *in2) {
  int a, b;
  a = in2[0];
  b = in2[1];
  printf("a is %d\n", a);
  printf("b is %d\n", b);
  if (a < b) {
    out2[0] = in2[0];
    out2[1] = in2[1];
  } else {
    out2[0] = in2[1];
    out2[1] = in2[0];
  }
  return;
}

void sort3(int *out3, int *in3) {
  sort2(out3,in3);
  in3[1] = out3[1];
  sort2(out3+1,in3+1);
  in3[0] = out3[0];
  in3[1] = out3[1];
  sort2(out3,in3);
}

CT_FUZZ_SPEC(void, sort3, int* out, int* in) {
  size_t out_len = __ct_fuzz_get_arr_len(out);
  size_t in_len = __ct_fuzz_get_arr_len(in);
  CT_FUZZ_ASSUME(out_len >=3 && out_len <= 10);
  CT_FUZZ_ASSUME(in_len >=3 && in_len <= 10);
  printf("%u: %d, %d, %d\n", in_len, in[0], in[1], in[2]);
}

CT_FUZZ_SEED(void, sort3,int*, int*) {
  SEED_1D_ARR(int, o1, 3, {0})
  SEED_1D_ARR(int, i1, 3, {1,2,3})
  SEED_1D_ARR(int, o2, 4, {0})
  SEED_1D_ARR(int, i2, 4, {1,2,3,4})
  SEED_2D_ARR(int, o, 2, 3, {{0}, {0}})
  PRODUCE(sort3, o1, i1)
  PRODUCE(sort3, &o[0], i2);
}
