// RUN: ct-fuzz %s --entry-point=foo -o %t
// RUN: %t < %s.hex > %t1
// RUN: diff %t1 %s.output
#include "ct-fuzz.h"

CT_FUZZ_SPEC(int, foo, short* a, unsigned len) {
  for (unsigned i = 0; i < len; ++i)
    printf(" %hx ", a[i]);
  printf("\n");
  __ct_fuzz_ptr_len(a, len, 2);
  __ct_fuzz_public_in(a);
}

int foo(short* a, unsigned len) {
  if (*a) {
	  return *a + 1;
  } else
    return 0;
}
