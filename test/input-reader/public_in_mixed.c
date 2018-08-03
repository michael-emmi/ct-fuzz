// RUN: ct-fuzz %s --entry-point=foo -o %t
// RUN: %t < %s.hex > %t1
// RUN: diff %t1 %s.output
#include "ct-fuzz.h"

CT_FUZZ_SPEC(int, foo, char* a, short b, char c) {
  __ct_fuzz_ptr_len(a, 2, 2);
  __ct_fuzz_public_in(a);
  __ct_fuzz_public_in(c);
}

int foo(char* a, short b, char c) {
  printf("public elements in a: %c..%c\n", a[0], a[1]);
  printf("b is %d\n", b);
  printf("c is %d\n", c);
  return *a + b + c;
}
