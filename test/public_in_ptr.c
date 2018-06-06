#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "ct-fuzz.h"

CT_FUZZ_SPEC(foo)(char* a) {
  __ct_fuzz_public_in(a, 2);
}

int foo(char* a) {
  printf("%lu\n", (unsigned long)malloc(1));
  printf("We're in: %d\n", getpid());
  printf("%c..%c\n", a[0], a[1]);
  return *a + 1;
}
