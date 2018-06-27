// RUN: ../../bin/ct-fuzz %s --entry-point=foo -o %t --compiler-options="-I."
// RUN: %t < %s.hex > %t1
// RUN: diff %t1 %s.output

#include <stdio.h>
#include "ct-fuzz-test.h"

CT_FUZZ_SPEC(int, foo, long x, int* y, short** z, char*** c) {
  printf("%ld\n", x);
  PRINT_LEN(y);
  PRINT_LEN(z);
  PRINT_LEN(*z);
  PRINT_LEN(c);
  PRINT_LEN(*c);
  PRINT_LEN(**c);
}

int foo(long x, int* y, short** z, char*** c) {
  printf("%ld\n", x);
  PRINT_LEN(y);
  PRINT_LEN(z);
  PRINT_LEN(*z);
  PRINT_LEN(c);
  PRINT_LEN(*c);
  PRINT_LEN(**c);
  return *y + 1;
}
