// RUN: ct-fuzz %s --entry-point=foo -o %t --seed-file %t1
// RUN: seq 2 | xargs -i -- cat %t1 | xargs -0 echo -n -e | tr -d '\n' | %t > %t2
// RUN: diff %t2 %s.output
#include <stdio.h>
#include "ct-fuzz.h"

long foo(short a, int b, long c) {
  return a + b + c;
}

CT_FUZZ_SPEC(long, foo, short a, int b, long c) {
  printf("%hd, %d, %ld\n", a, b, c);
}

CT_FUZZ_SEED(long, foo, short, int, long) {
  SEED_UNIT(int, a, 3)
  SEED_UNIT(long, c, 5)
  PRODUCE(foo,a,4,c);
}
