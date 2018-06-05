#include <stdio.h>
#include "ct-fuzz.h"

CT_FUZZ_SPEC(foo)(char* a) {
}

int foo(char* a) {
  printf("%c..%c\n", a[0], a[1]);
  return *a + 1;
}
