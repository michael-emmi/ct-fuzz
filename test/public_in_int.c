#include <stdio.h>
#include "ct-fuzz.h"

CT_FUZZ_SPEC(foo)(int x) {
  __ct_fuzz_public_in(x%2);
}

int foo(int x) {
  printf("%d\n", x);
  return x + 1;
}
