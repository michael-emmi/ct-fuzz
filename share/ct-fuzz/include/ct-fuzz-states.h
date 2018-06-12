#ifndef CT_FUZZ_STATES
#define CT_FUZZ_STATES

#include <sys/mman.h>
#include "ct-fuzz-utils.h"

// unhealthy but necessary?
#define HASH_T TYPE(hash)
#define IDX_T TYPE(idx)
#define MONITORS PREFIX(monitors)
#define RUN_ID PREFIX(run_idx)

typedef unsigned long long HASH_T; 
typedef unsigned char IDX_T;

HASH_T* MONITORS;
IDX_T RUN_ID;

void* PREFIX(create_shared_memory)(size_t size) {
  return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
}

void PREFIX(initialize_states)() {
  MONITORS = (HASH_T*)PREFIX(create_shared_memory)(2*sizeof(HASH_T));
  MONITORS[0] = MONITORS[1] = 0;
}

#endif
