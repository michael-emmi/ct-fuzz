// RUN: ct-fuzz %s --entry-point=foo -o %t --compiler-options="-I../"
// RUN: %t < %s.hex.match > %t1
// RUN: %t < %s.hex.mismatch > %t2 || true
// RUN: diff %t1 %s.output.match && diff %t2 %s.output.mismatch
#include "ct-fuzz-test.h"

CT_FUZZ_SPEC(int, foo, char* a) {
  unsigned short len = __ct_fuzz_get_arr_len(a);
  PRINT_LEN(a);
  for (unsigned i = 0; i < len; ++i)
    printf(" %x ", a[i]);
  printf("\n");
  CT_FUZZ_ASSUME(2 <= len && len < 10);
  __ct_fuzz_public_in(a, 2);
}

int foo(char* a) {
  if (*a) {
	  printf("%c..%c\n", a[0], a[1]);
	  return *a + 1;
  } else
    return 0;
}
