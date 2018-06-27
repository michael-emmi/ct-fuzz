#include <signal.h>
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>
#include "xxhash.h"
#include "ct-fuzz-observation.h"
#include "ct-fuzz-debug.h"

#define STOP_SIGNAL 42
#define HASH_T TYPE(hash)
#define MONITORS NS(monitors)

typedef unsigned long long HASH_T;

HASH_T* MONITORS;
IDX_T RUN_ID;

void* NS(create_shared_memory)(size_t size) {
  return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
}

void NS(initialize_states)() {
  MONITORS = (HASH_T*)NS(create_shared_memory)(2*sizeof(HASH_T));
  MONITORS[0] = MONITORS[1] = 0;
}

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
