#include <unistd.h>
#include <stddef.h>
#include <stdint.h>
#include "ct-fuzz-utils.h"

#define ARR_INFO_SIZE 100
#define ARR_INFO_T PREFIX(arr_info)
#define ARR_INFO_COUNTER PREFIX(arr_info_counter)
#define ARR_INFO_LIST PREFIX(array_info_list)

typedef uint16_t len_t;
typedef void (*func_t)(char*);

extern len_t PREFIX(read_array_len)(void);
extern len_t PREFIX(get_arr_len)(char*);
extern void PREFIX(stdin_read)(void* buf, size_t size);
extern void PREFIX(read_ptr_generic)(char**, size_t, unsigned, func_t);
