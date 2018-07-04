#ifndef CT_FUZZ_DEBUG
#define CT_FUZZ_DEBUG

#include <stdbool.h>
#include <stdint.h>
#include "ct-fuzz-utils.h"

#define DEBUG_ON NS(debug_on)
#define DEBUG_PRINT NS(dbg_print)
#define DEBUG_PRINT_MSG NS(dbg_print_msg)

typedef uint32_t num_t;

extern void NS(dbg_print_cond)(bool cond, char* fn, num_t ln, num_t cn);
extern void NS(dbg_print_addr)(char* addr, char* fn, num_t ln, num_t cn);
extern void DEBUG_PRINT_MSG(char* s);
extern void NS(dbg_init)();
#endif
