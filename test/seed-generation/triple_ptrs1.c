// RUN: ct-fuzz %s --entry-point=foo -o %t --seed-file %t1
// RUN: seq 2 | xargs -i -- cat %t1 | xargs -0 echo -n -e | tr -d '\n' | %t > %t2
// RUN: diff %t2 %s.output
#include <stdio.h>
#include "ct-fuzz.h"

#define LEN 2
void foo(int*** a) {
  a[0][0][0] = 1;
}

CT_FUZZ_SPEC(void, foo, int*** a) {
  unsigned short len1 = __ct_fuzz_get_arr_len(a);
  unsigned short len2 = __ct_fuzz_get_arr_len(*a);
  unsigned short len3 = __ct_fuzz_get_arr_len(**a);
  for (unsigned short i = 0; i < len1; ++i)
    for (unsigned short j = 0; j < len2; ++j)
      for (unsigned short k = 0; k < len3; ++k)
        printf("%d,", a[i][j][k]);
  printf("\n");
}

CT_FUZZ_SEED(void, foo, int***) {
  SEED_3D_ARR(int, j, LEN, LEN, LEN, {{{5, 6},{7,8}}, {{0, 1},{2,3}}})
  PRODUCE(foo, j);
}
