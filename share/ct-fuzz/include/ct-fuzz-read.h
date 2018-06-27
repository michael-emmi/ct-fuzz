#include <unistd.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "ct-fuzz-utils.h"

#define ARR_INFO_SIZE 100
#define ARR_INFO_T NS(arr_info)
#define ARR_INFO_COUNTER NS(arr_info_counter)
#define ARR_INFO_LIST NS(array_info_list)

typedef uint16_t len_t;
typedef void (*read_func_t)(char*);
typedef void (*merge_func_t)(char*, char*, char*);
typedef void (*deep_copy_func_t)(char*, char*);

extern len_t NS(read_array_len)(void);
extern len_t NS(get_arr_len)(const char*);
extern void NS(stdin_read)(void* buf, size_t size);
extern void NS(read_ptr_generic)(char**, size_t, unsigned, read_func_t);

//======================================================
extern void NS(stdout_write)(char*, size_t);
extern void NS(write_ptr_generic)(unsigned, generate_func_t);

//======================================================
