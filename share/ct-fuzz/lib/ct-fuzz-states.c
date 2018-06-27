#include "ct-fuzz-states.h"

HASH_T* MONITORS;
IDX_T RUN_ID;

void* NS(create_shared_memory)(size_t size) {
  return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
}

void NS(initialize_states)() {
  MONITORS = (HASH_T*)NS(create_shared_memory)(2*sizeof(HASH_T));
  MONITORS[0] = MONITORS[1] = 0;
}
