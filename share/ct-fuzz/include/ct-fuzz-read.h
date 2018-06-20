#include <unistd.h>
#include <stddef.h>
#include <stdint.h>
#include "ct-fuzz-utils.h"

typedef uint16_t len_t;
typedef void (*func_t)(char*);

extern void PREFIX(stdin_read)(void* buf, size_t size);
extern len_t PREFIX(read_array_len)(void);
extern void PREFIX(read_ptr_generic)(char**, size_t, unsigned, func_t);
