// RUN: ../../bin/ct-fuzz %s --entry-point=foo -o %t --compiler-options="-I../"
// RUN: %t < %s.hex > %t1
// RUN: diff %t1 %s.output

#include <stdio.h>
#include "ct-fuzz-test.h"

CT_FUZZ_SPEC(int, foo, int x, int* y) {
  printf("%d\n", x);
  PRINT_LEN(y);
  __ct_fuzz_public_in(x * y[0]);
  __ct_fuzz_public_in(y, 1);
}

int foo(int x, int* y) {
  printf("%d\n", x);
  PRINT_LEN(y);
  return *y + 1;
}
