// RUN: ../../bin/ct-fuzz %s --entry-point=foo -o %t
// RUN: %t < %s.hex > %t1
// RUN: diff %t1 %s.output

#include <stdio.h>
#include "ct-fuzz.h"

CT_FUZZ_SPEC(int, foo, long x, int y, short z, char c) {
  printf("%ld..%d..%hd..%c\n", x, y, z, c);
}

int foo(long x, int y, short z, char c) {
  printf("%ld..%d..%hd..%c\n", x, y, z, c);
  return y + 1;
}
