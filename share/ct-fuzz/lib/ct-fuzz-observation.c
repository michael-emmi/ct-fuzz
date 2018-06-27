#include "ct-fuzz-observation.h"

static void NS(update_hash)(char* buf, size_t size) {
  HASH_T* old = &MONITORS[RUN_ID];
  char temp[sizeof(HASH_T)+size];
  memcpy(temp, old, sizeof(HASH_T));
  memcpy(temp+sizeof(HASH_T), buf, size);
  *old = XXH64((void*)&temp, sizeof(temp), 0);
}

void NS(update_monitor_by_cond)(bool cond) {
  NS(dbg_print_cond)(cond);
  NS(update_hash)((char*)&cond, sizeof(bool));
}

void NS(update_monitor_by_addr)(char* addr) {
  NS(dbg_print_addr)(addr);
  NS(update_hash)((char*)&addr, sizeof(char*));
}

void NS(check_observations)() {
  if (MONITORS[0] != MONITORS[1]) {
    //int* p;
    //DEBUG_PRINT_MSG("oops");
    //*p = 42;
    DEBUG_PRINT_MSG("oops");
    raise(STOP_SIGNAL);
  }
}
