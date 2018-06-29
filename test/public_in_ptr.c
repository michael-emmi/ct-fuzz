#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "ct-fuzz.h"
//echo -n -e '\x02\x00\x00\x00\x00\x00\x00\x00\x30\x31\x03\x00\x00\x00\x00\x00\x00\x00\x30\x31\x34' | ./a.out

CT_FUZZ_SPEC(foo)(char* a) {
  size_t len = __ct_fuzz_array_len(a);
  printf("length is %lu\n", (unsigned long)len);
  for (unsigned i = 0; i < len; ++i)
    printf(" %x ", a[i]);
  printf("\n");
  CT_FUZZ_ASSUME(2 <= len && len < 10);
  //__ct_fuzz_public_in(a, 2);
}

int foo(char* a) {
  printf("%lu\n", (unsigned long)malloc(1));
  printf("We're in: %d\n", getpid());
  if (*a) {
	  printf("%c..%c\n", a[0], a[1]);
	  return *a + 1;
  } else
    return 0;
}
