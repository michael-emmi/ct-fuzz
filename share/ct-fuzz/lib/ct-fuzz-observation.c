#include <signal.h>
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>
#include "xxhash.h"
#include "ct-fuzz-observation.h"
#include "ct-fuzz-debug.h"

#define STOP_SIGNAL 42
#define HASH_T TYPE(hash)
#define COUNTER_T TYPE(counter)
#define MONITORS NS(monitors)
#define COUNTERS NS(counters)

typedef unsigned long long HASH_T;
typedef unsigned long COUNTER_T;

HASH_T* MONITORS;
COUNTER_T* COUNTERS;
IDX_T RUN_ID;
bool START_OB;

void* NS(create_shared_memory)(size_t size) {
  return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
}

void NS(initialize_states)() {
  MONITORS = (HASH_T*)NS(create_shared_memory)(2*sizeof(HASH_T));
  COUNTERS = (COUNTER_T*)NS(create_shared_memory)(2*sizeof(COUNTER_T));
  MONITORS[0] = MONITORS[1] = 0;
  COUNTERS[0] = COUNTERS[1] = 0;
}

static void NS(update_hash)(char* buf, size_t size) {
  if (START_OB) {
    HASH_T* old = &MONITORS[RUN_ID];
    char temp[sizeof(HASH_T)+size];
    memcpy(temp, old, sizeof(HASH_T));
    memcpy(temp+sizeof(HASH_T), buf, size);
    *old = XXH64((void*)&temp, sizeof(temp), 0);
    COUNTERS[RUN_ID]++;
  }
}

void NS(update_monitor_by_cond)(bool cond, char* fn, num_t ln, num_t cn) {
  NS(dbg_print_cond)(cond, fn, ln, cn);
  NS(update_hash)((char*)&cond, sizeof(bool));
}

void NS(update_monitor_by_addr)(char* addr, char* fn, num_t ln, num_t cn) {
  NS(dbg_print_addr)(addr, fn, ln, cn);
  NS(update_hash)((char*)&addr, sizeof(char*));
}

void NS(check_observations)() {
  if (COUNTERS[0] != COUNTERS[1]
    || MONITORS[0] != MONITORS[1]) {
    //int* p;
    //DEBUG_PRINT_MSG("oops");
    //*p = 42;
    DEBUG_PRINT_MSG("oops");
    raise(STOP_SIGNAL);
  }
}
