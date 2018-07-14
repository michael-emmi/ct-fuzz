// RUN: ct-fuzz %s --entry-point=foo -o %t --seed-file %t1
// RUN: seq 2 | xargs -i -- cat %t1 | xargs -0 echo -n -e | tr -d '\n' | %t > %t2
// RUN: diff %t2 %s.output
#include <stdio.h>
#include "ct-fuzz.h"

#define LEN 2
void foo(int* a) {
  a[0] = 1;
}

CT_FUZZ_SPEC(void, foo, int* a) {
  unsigned short len = __ct_fuzz_get_arr_len(a);
  for (unsigned short i = 0; i < len; ++i)
    printf("%d,", a[i]);
  printf("\n");
}

CT_FUZZ_SEED(void, foo, int*) {
  SEED_1D_ARR(int, i, LEN, {2, 1})
  PRODUCE(foo, i);
}
