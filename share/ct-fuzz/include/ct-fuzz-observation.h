#ifndef CT_FUZZ_OBSERVATION
#define CT_FUZZ_OBSERVATION

#include <unistd.h>
#include "ct-fuzz-utils.h"
#include "ct-fuzz-debug.h"

#define IDX_T TYPE(idx)
#define RUN_ID NS(run_idx)

typedef unsigned char IDX_T;

extern IDX_T RUN_ID;

extern void NS(initialize_states)();
extern void NS(update_monitor_by_cond)(bool cond);
extern void NS(update_monitor_by_addr)(char* addr);
extern void NS(check_observations)();

#endif
