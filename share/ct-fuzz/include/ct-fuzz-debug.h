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

bool DEBUG_ON;

void DEBUG_PRINT(char* type, char* value) { printf("[dbg] [%u] [%s, %s]\n", RUN_ID, type, value); }

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

void DEBUG_PRINT_MSG(char* s) {
  if (DEBUG_ON) {
    printf("[dbg] %s", s);
    printf("\n");
  }
}

void PREFIX(dbg_init)() {
  DEBUG_ON = getenv("DEBUG");
  DEBUG_PRINT_MSG("debugging");
}

#endif
