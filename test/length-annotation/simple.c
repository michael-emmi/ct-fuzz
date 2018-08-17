// RUN: ct-fuzz %s --entry-point=foo -o %t --seed-file %t1
// RUN: seq 2 | xargs -i -- cat %t1 | xargs -0 echo -n -e | tr -d '\n' | %t > %t2
// RUN: diff %t2 %s.output
#include <string.h>
#include <stdio.h>
#include "ct-fuzz.h"

void foo(int* a, unsigned len) {
  printf("a(%u) is: ", len);
  for (unsigned i = 0; i < len; ++i) 
    printf("%d, ", a[i]);
  printf("\n");
}

CT_FUZZ_SPEC(void, foo, int* a, unsigned len) {
  printf("len is %u\n", len);
  __ct_fuzz_ptr_len(a, len, 4);
  __ct_fuzz_public_in(len);
}

CT_FUZZ_SEED(void, foo, int*, unsigned) {
  SEED_1D_ARR(int, i, 4, {3, 4, 2, 1})
  PRODUCE(foo, i, 9);
}
