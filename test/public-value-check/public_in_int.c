// RUN: ct-fuzz %s --entry-point=foo -o %t
// RUN: %t < %s.hex.match > %t1
// RUN: %t < %s.hex.mismatch > %t2 || true
// RUN: diff %t1 %s.output.match && diff %t2 %s.output.mismatch
#include <stdio.h>
#include "ct-fuzz.h"

CT_FUZZ_SPEC(int, foo, int x, char y) {
  __ct_fuzz_public_in(x%2);
  __ct_fuzz_public_in(x+y);
}

int foo(int x, char y) {
  printf("%d..%d\n", x, y);
  return x + 1;
}
