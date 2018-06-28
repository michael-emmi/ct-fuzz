#include <string.h>
#include "ct-fuzz.h"

#define PRINT_LEN(X) print_len(#X, X)

void print_len(const char* id, char* ptr) {
  char buf[64];
  sprintf(buf, "The length of %s is %hu", id, __ct_fuzz_get_arr_len(ptr));
  printf("%s\n", buf);
}


