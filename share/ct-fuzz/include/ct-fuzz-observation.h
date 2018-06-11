#ifndef CT_FUZZ_OBSERVATION
#define CT_FUZZ_OBSERVATION

#include <unistd.h>
#include <stdio.h>
#include "xxhash.h"
#include "ct-fuzz-utils.h"
#include "ct-fuzz-states.h"
#include "ct-fuzz-debug.h"

void PREFIX(update_hash)(char* buf, size_t size) {
  HASH_T* old = &MONITORS[RUN_ID];
  char temp[sizeof(HASH_T)+size];
  memcpy(temp, old, sizeof(HASH_T));
  memcpy(temp+sizeof(HASH_T), buf, size);
  *old = XXH64((void*)&temp, sizeof(temp), 0);
}

void PREFIX(update_monitor_by_cond)(bool cond) {
  PREFIX(update_hash)((char*)&cond, sizeof(bool));
}

void PREFIX(update_monitor_by_addr)(char* addr) {
  PREFIX(update_hash)(addr, sizeof(char*));
}

void PREFIX(check_observations)() {
  if (MONITORS[0] != MONITORS[1]) {
    int* p;
    printf("[dbg] oops\n");
    *p = 42;
  }
}

#endif
