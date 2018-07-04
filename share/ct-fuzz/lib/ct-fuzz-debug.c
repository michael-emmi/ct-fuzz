#include <stdio.h>
#include <stdlib.h>
#include "ct-fuzz-observation.h"
#include "ct-fuzz-debug.h"

static bool DEBUG_ON;

static void DEBUG_PRINT(char* type, char* value, char* fn, num_t ln, num_t cn) {
  printf("[dbg] [%u] [%s: %u, %u] [%s, %s]\n",
    RUN_ID, fn, ln, cn, type, value);
}

void NS(dbg_print_cond)(bool cond, char* fn, num_t ln, num_t cn) {
  if (DEBUG_ON)
    DEBUG_PRINT("condition",
      cond? "true" : "false",
      fn, ln, cn);
}

void NS(dbg_print_addr)(char* addr, char* fn, num_t ln, num_t cn) {
  if (DEBUG_ON) {
    char s[40];
    sprintf(s, "%lx", (unsigned long)addr);
    DEBUG_PRINT("address", s, fn, ln, cn);
  }
}

void DEBUG_PRINT_MSG(char* s) {
  if (DEBUG_ON) {
    printf("[dbg] %s", s);
    printf("\n");
  }
}

void NS(dbg_init)() {
  DEBUG_ON = getenv("DEBUG");
  DEBUG_PRINT_MSG("debugging");
}
