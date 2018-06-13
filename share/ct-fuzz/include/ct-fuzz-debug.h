#ifndef CT_FUZZ_DEBUG
#define CT_FUZZ_DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "ct-fuzz-utils.h"
#include "ct-fuzz-states.h"

#define DEBUG_ON PREFIX(debug_on)
#define DEBUG_PRINT PREFIX(dbg_print)
#define DEBUG_PRINT_MSG PREFIX(dbg_print_msg)

extern void PREFIX(dbg_print_cond)(bool cond);
extern void PREFIX(dbg_print_addr)(char* addr);
extern void DEBUG_PRINT_MSG(char* s);
extern void PREFIX(dbg_init)();
#endif
