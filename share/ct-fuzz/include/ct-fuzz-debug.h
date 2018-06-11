#ifndef CT_FUZZ_DEBUG
#define CT_FUZZ_DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "ct-fuzz-utils.h"

#define DEBUG_ON PREFIX(debug_on)
#define DEBUG_PRINT PREFIX(dbg_print)

bool DEBUG_ON;

void PREFIX(dbg_print)(char* type, char* value) { printf("[dbg] [%s, %s]\n", type, value); }

void PREFIX(dbg_print_cond)(bool cond) {
  if (DEBUG_ON)
    DEBUG_PRINT("condition", cond? "true" : "false");
}

void PREFIX(dbg_print_addr)(char* addr) {
  char s[40];
  if (DEBUG_ON) {
    sprintf(s, "%lx", (unsigned long)addr);
    DEBUG_PRINT("address", s);
  }
}

void PREFIX(dbg_init)() {
  DEBUG_ON = getenv("DEBUG");
  if (DEBUG_ON) printf("[dbg] debugging\n");
}

#endif
