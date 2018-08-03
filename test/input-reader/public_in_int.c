// RUN: ct-fuzz %s --entry-point=foo -o %t
// RUN: %t < %s.hex > %t1
// RUN: diff %t1 %s.output
#include <stdio.h>
#include "ct-fuzz.h"

CT_FUZZ_SPEC(int, foo, int x, char y) {
  __ct_fuzz_public_in(x);
  __ct_fuzz_public_in(y);
}

int foo(int x, char y) {
  printf("%d..%d\n", x, y);
  return x + 1;
}
