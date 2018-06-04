#include <stdio.h>
#include "ct-fuzz.h"

CT_FUZZ_SPEC(foo)(int x, char y) {
  __ct_fuzz_public_in(x%2);
  __ct_fuzz_public_in(x+y);
}

int foo(int x, char y) {
  printf("%d..%d\n", x, y);
  return x + 1;
}
