#ifndef CT_FUZZ_OBSERVATION
#ifdef CT_FUZZ_OBSERVATION

#include <unistd.h>
#include <stdio.h>
#include "xxhash.h"
#include "ct-fuzz-utils.h"
#include "ct-fuzz-states.h"

void PREFIX(update_monitor_by_cond)(bool cond) {
  PREFIX(update_hash)(&cond, sizeof(bool));
}

void PREFIX(update_monitor_by_addr)(char* addr) {
  PREFIX(update_hash)(addr, sizeof(char*));
}

void PREFIX(update_hash)(char* buf, size_t size) {
  HASH_T* old = &MINITORS[RUN_ID];
  char temp[sizeof(HASH_T)+size];
  memcpy(temp, old, sizeof(HASH_T));
  memcpy(temp+sizeof(HASH_T), &addr, size);
  *old = XXH64((void*)&temp, sizeof(temp), 0);
}

void PREFIX(check_observations)() {
  if (MONITORS[0] != MONITORS[1]) {
    int* p;
    *p = 42;
  }
}

#endif
