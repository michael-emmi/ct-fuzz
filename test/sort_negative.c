#include "ct-fuzz.h"
#include <stdio.h>

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
  size_t out_len = __ct_fuzz_array_len(out);
  size_t in_len = __ct_fuzz_array_len(in);
  CT_FUZZ_ASSUME(out_len >=3 && out_len <= 10);
  CT_FUZZ_ASSUME(in_len >=3 && in_len <= 10);
  printf("%u: %d, %d, %d\n", in_len, in[0], in[1], in[2]);
}
