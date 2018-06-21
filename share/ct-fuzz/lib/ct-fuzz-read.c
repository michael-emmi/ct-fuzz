#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "ct-fuzz-read.h"

typedef struct {
  char* ptr;
  len_t len;
} ARR_INFO_T;

static ARR_INFO_T* ARR_INFO_LIST[ARR_INFO_SIZE];
static unsigned char ARR_INFO_COUNTER = 0;

static void PREFIX(set_arr_len)(char* ptr, len_t len) {
  ARR_INFO_T* ai = (ARR_INFO_T*)malloc(sizeof(ARR_INFO_T));
  ai->ptr = ptr;
  ai->len = len;
  ARR_INFO_LIST[ARR_INFO_COUNTER++] = ai;
}

len_t PREFIX(get_arr_len)(char* ptr) {
  for (unsigned char i = 0; i < ARR_INFO_COUNTER; ++i)
    if (ARR_INFO_LIST[i]->ptr == ptr)
      return ARR_INFO_LIST[i]->len;
  assert(0 && "not a valid pointer to query array length");
}

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
  //printf("len: %u\n", len);
  *ppt = malloc(rs*len);
  PREFIX(set_arr_len)(*ppt, len);
  //printf("indirection: %u\n", indirection);
  //printf("element size: %u\n", es);
  for (len_t i = 0; i < len; ++i)
    if (indirection)
      PREFIX(read_ptr_generic)(*ppt + rs*i, es, --indirection, callback);
    else
      (*callback)(*ppt + rs*i);
}
