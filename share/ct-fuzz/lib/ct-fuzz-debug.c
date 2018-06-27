#include "ct-fuzz-debug.h"
static bool DEBUG_ON;

void DEBUG_PRINT(char* type, char* value) { printf("[dbg] [%u] [%s, %s]\n", RUN_ID, type, value); }

void NS(dbg_print_cond)(bool cond) {
  if (DEBUG_ON)
    DEBUG_PRINT("condition", cond? "true" : "false");
}

void NS(dbg_print_addr)(char* addr) {
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

void NS(dbg_init)() {
  DEBUG_ON = getenv("DEBUG");
  DEBUG_PRINT_MSG("debugging");
}
