// RUN: ct-fuzz %s --entry-point=foo -o %t --seed-file %t1
// RUN: seq 2 | xargs -i -- cat %t1 | xargs -0 echo -n -e | tr -d '\n' | %t > %t2
// RUN: diff %t2 %s.output
#include <stdio.h>
#include "ct-fuzz.h"

#define INT_LEN 2
#define SHORT_LEN 4
#define CHAR_LEN 8

void foo(int* a, short* b, char* c) {
  a[0] = 1;
}

CT_FUZZ_SPEC(void, foo, int* a, short* b, char* c) {
  __ct_fuzz_ptr_len(a, INT_LEN, INT_LEN);
  __ct_fuzz_ptr_len(b, SHORT_LEN, SHORT_LEN);
  __ct_fuzz_ptr_len(c, CHAR_LEN, CHAR_LEN);
  for (unsigned short i = 0; i < INT_LEN; ++i)
    printf("%d,", a[i]);
  printf("\n");
  for (unsigned short i = 0; i < SHORT_LEN; ++i)
    printf("%d,", b[i]);
  printf("\n");
  for (unsigned short i = 0; i < CHAR_LEN; ++i)
    printf("%d,", c[i]);
  printf("\n");
}

CT_FUZZ_SEED(void, foo, int*, short*, char*) {
  SEED_1D_ARR(int, a, INT_LEN, {1, 1})
  SEED_1D_ARR(short, b, SHORT_LEN, {2, 2, 2, 2})
  SEED_1D_ARR(char, c, CHAR_LEN, {3, 3, 3, 3, 3, 3, 3, 3})
  PRODUCE(foo, a, b, c);
}
