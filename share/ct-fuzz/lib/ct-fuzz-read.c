#include <stdlib.h>
#include <stdio.h>
#include "ct-fuzz-read.h"

void PREFIX(stdin_read)(void* buf, size_t size) {
  // not sure if the if condition is required.
  // it's added just to make sure nothing bad happens.
  if (size)
    read(0, buf, size);
}

len_t PREFIX(read_array_len)(void) {
  len_t ret;
  PREFIX(stdin_read)(&ret, sizeof(len_t));
  return ret;
}

void PREFIX(read_ptr_generic)(char** ppt, size_t es, unsigned indirection, func_t callback) {
  len_t len = PREFIX(read_array_len)();
  size_t rs = (indirection? sizeof(char*) : es);
  printf("len: %u\n", len);
  *ppt = malloc(rs*len);
  printf("indirection: %u\n", indirection);
  printf("element size: %u\n", es);
  for (len_t i = 0; i < len; ++i)
    if (indirection)
      PREFIX(read_ptr_generic)(*ppt + rs*i, es, --indirection, callback);
    else
      (*callback)(*ppt + rs*i);
}
