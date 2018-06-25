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
  for (unsigned char i = 0; i < ARR_INFO_COUNTER; ++i)
    if (ARR_INFO_LIST[i]->ptr == ptr)
      return ARR_INFO_LIST[i]->len;
  assert(0 && "not a valid pointer to query array length");
}

len_t PREFIX(max_len)(len_t a, len_t b) {
  //printf("size 1 is: %u\n", a);
  //printf("size 2 is: %u\n", b);
  if (!a || !b) {
    // be demonic here: if either one has zero-sized memory,
    // make both pointer arugments null.
    return 0;
  }
  else if (a > b)
    return a;
  else
    return b;
}

len_t PREFIX(min_len)(len_t a, len_t b) {
  //printf("size 1 is: %u\n", a);
  //printf("size 2 is: %u\n", b);
  if (!a || !b) {
    // be demonic here: if either one has zero-sized memory,
    // make both pointer arugments null.
    return 0;
  }
  else if (a > b)
    return b;
  else
    return a;
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
  for (len_t i = 0; i < len; ++i)
    if (indirection)
      PREFIX(read_ptr_generic)(*ppt + rs*i, es, indirection - 1, callback);
    else
      (*callback)(*ppt + rs*i);
}

//===========================================================
/*
#define ARR_LEN_MAX 8

void PREFIX(stdout_write)(char* buf, size_t size) {
  char tmp[64];
  for (size_t i = 0; i < size; ++i)
    sprintf(tmp+4*i, "\\x%02X", buf[i]);
  write(1, tmp, size*4);
}

static len_t PREFIX(generate_array_len)(void) {
  if (rand() & 1U)
    return 1;
  else
    return (rand() % ARR_LEN_MAX) + 2;
}

void PREFIX(generate_ptr_generic)(unsigned indirection, generate_func_t callback) {
  len_t len = PREFIX(generate_array_len)();
  // write length first
  PREFIX(stdout_write)(&len, sizeof(len_t));
  for (len_t i = 0; i < len; ++i)
    if (indirection)
      PREFIX(generate_ptr_generic)(indirection - 1, callback);
    else
      (*callback)();
}
*/

//===========================================================
void PREFIX(merge_ptr_generic)(char**ppt, char** ppt_1, char** ppt_2, size_t es, unsigned indirection, merge_func_t callback) {
  len_t len_1 = PREFIX(get_arr_len)(*ppt_1);
  len_t len_2 = PREFIX(get_arr_len)(*ppt_2);
  len_t max_len = PREFIX(max_len)(len_1, len_2);
  size_t rs = (indirection? sizeof(char*) : es);
  *ppt = malloc(rs*max_len);
  for (len_t i = 0; i < PREFIX(min_len)(len_1, len_2); ++i) {
    if (indirection)
      PREFIX(merge_ptr_generic)(*ppt + rs*i, *ppt_1 + rs*i, *ppt_2 + rs*i, es, indirection - 1, callback);
    else
      (*callback)(*ppt + rs*i, *ppt_1 + rs*i, *ppt_2 + rs*i);
  }
}

//==========================================================
void PREFIX(deep_copy_ptr_generic)(char** dest, char** src, size_t es, unsigned indirection, deep_copy_func_t callback) {
  len_t len = PREFIX(get_arr_len)(*src);
  size_t rs = (indirection? sizeof(char*) : es);
  for (len_t i = 0; i < len; ++i) {
    if (indirection)
      PREFIX(deep_copy_ptr_generic)(*dest + rs*i, *src + rs*i, es, indirection - 1, callback);
    else
      (*callback)(*dest + rs*i, *src + rs*i);
  }
}
