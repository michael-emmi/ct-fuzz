#ifndef CT_FUZZ_STATES
#define CT_FUZZ_STATES

#include <sys/mman.h>
#include <stdlib.h>
#include "ct-fuzz-utils.h"

// unhealthy but necessary?
#define HASH_T TYPE(hash)
#define IDX_T TYPE(idx)
#define MONITORS PREFIX(monitors)
#define RUN_ID PREFIX(run_idx)

typedef unsigned long long HASH_T; 
typedef unsigned char IDX_T;

extern HASH_T* MONITORS;
extern IDX_T RUN_ID;

extern void PREFIX(initialize_states)(); 

#endif
