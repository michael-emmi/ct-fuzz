#ifndef CT_FUZZ_STATES
#define CT_FUZZ_STATES

#include <sys/mman.h>
#include <stdlib.h>
#include "ct-fuzz-utils.h"

#define HASH_T TYPE(hash)
#define IDX_T TYPE(idx)
#define MONITORS NS(monitors)
#define RUN_ID NS(run_idx)

typedef unsigned long long HASH_T;
typedef unsigned char IDX_T;

extern HASH_T* MONITORS;
extern IDX_T RUN_ID;

extern void NS(initialize_states)();

#endif
