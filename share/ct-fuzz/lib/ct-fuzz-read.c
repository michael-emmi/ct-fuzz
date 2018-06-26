#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
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

len_t PREFIX(get_arr_len)(const char* ptr) {
  assert(ptr && "ptr can't be null");
  for (unsigned char i = 0; i < ARR_INFO_COUNTER; ++i)
    if (ARR_INFO_LIST[i]->ptr == ptr)
      return ARR_INFO_LIST[i]->len;
  printf("addr to query: %lx\n", (unsigned long)ptr);
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

void PREFIX(read_ptr_generic)(char** ppt, size_t es, unsigned indirection, read_func_t callback) {
  len_t len = PREFIX(read_array_len)();
  size_t rs = (indirection? sizeof(char*) : es);
  *ppt = malloc(rs*len);
  PREFIX(set_arr_len)(*ppt, len);
  for (len_t i = 0; i < len; ++i) {
    char* s = *ppt + rs*i;
    if (indirection)
      PREFIX(read_ptr_generic)(s, es, indirection - 1, callback);
    else
      (*callback)(s);
  }
}

//===========================================================
void PREFIX(merge_ptr_generic)(char**ppt, char** ppt_1, char** ppt_2, size_t es, unsigned indirection, merge_func_t callback) {
  len_t len_1 = PREFIX(get_arr_len)(*ppt_1);
  len_t len_2 = PREFIX(get_arr_len)(*ppt_2);
  len_t max_len = len_1 > len_2? len_1 : len_2;
  size_t rs = (indirection? sizeof(char*) : es);
  *ppt = malloc(rs*max_len);
  PREFIX(set_arr_len)(*ppt, max_len);
  for (len_t i = 0; i < max_len; ++i) {
    char* s = *ppt + rs*i;
    char* p = *ppt_1 + rs*i;
    char* q = *ppt_2 + rs*i;
    // the longer one merges with itself
    char* p1 = i < len_1? p : q;
    char* p2 = i < len_2? q : p;
    if (indirection)
      PREFIX(merge_ptr_generic)(s, p1, p2, es, indirection - 1, callback);
    else
      (*callback)(s, p1, p2);
  }
}

//==========================================================
void PREFIX(deep_copy_ptr_generic)(char** dest, char** src, size_t es, unsigned indirection, deep_copy_func_t callback) {
  len_t len = PREFIX(get_arr_len)(*src);
  size_t rs = (indirection? sizeof(char*) : es);
  for (len_t i = 0; i < len; ++i) {
    char* nd = *dest + rs*i;
    char* ns = *src + rs*i;
    if (indirection)
      PREFIX(deep_copy_ptr_generic)(nd, ns, es, indirection - 1, callback);
    else
      (*callback)(nd, ns);
  }
}
