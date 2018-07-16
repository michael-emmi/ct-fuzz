// RUN: ct-fuzz %s --entry-point=foo -o %t --compiler-options="-I../"
// RUN: %t < %s.hex.match > %t1
// RUN: %t < %s.hex.mismatch > %t2 || true
// RUN: diff %t1 %s.output.match && diff %t2 %s.output.mismatch
#include "ct-fuzz-test.h"

CT_FUZZ_SPEC(int, foo, char* a, short b, char c) {
  __ct_fuzz_public_in(a, 2);
  __ct_fuzz_public_in(b*c);
}

int foo(char* a, short b, char c) {
  printf("public elements in a: %c..%c\n", a[0], a[1]);
  printf("b is %d\n", b);
  printf("c is %d\n", c);
  return *a + b + c;
}
