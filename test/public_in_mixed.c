#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "ct-fuzz.h"
//echo -n -e '\x02\x00\x00\x00\x00\x00\x00\x00\x30\x31\x02\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x30\x31\x34\x00\x00\x01' | ./a.out

CT_FUZZ_SPEC(foo)(char* a, short b, char c) {
  __ct_fuzz_public_in(a, 2);
  __ct_fuzz_public_in(b*c);
}

int foo(char* a, short b, char c) {
  printf("malloc address is %lu\n", (unsigned long)malloc(sizeof(c)));
  printf("We're in: %d\n", getpid());
  printf("public elements in a: %c..%c\n", a[0], a[1]);
  printf("b is %d\n", b);
  printf("c is %d\n", c);
  return *a + b + c;
}
