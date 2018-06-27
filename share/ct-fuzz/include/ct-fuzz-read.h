#include "ct-fuzz-utils.h"
#include <stdint.h>

typedef uint16_t len_t;
typedef void (*read_func_t)(char*);
typedef void (*merge_func_t)(char*, char*, char*);
typedef void (*deep_copy_func_t)(char*, char*);

extern len_t NS(get_arr_len)(const char*);
extern void NS(stdin_read)(void* buf, size_t size);
extern void NS(read_ptr_generic)(char** ppt, size_t es, unsigned indirection, read_func_t callback);
extern void NS(merge_ptr_generic)(char**ppt, char** ppt_1, char** ppt_2, size_t es, unsigned indirection, merge_func_t callback);
extern void NS(deep_copy_ptr_generic)(char** dest, char** src, size_t es, unsigned indirection, deep_copy_func_t callback);
