#ifndef CT_FUZZ_OBSERVATION
#define CT_FUZZ_OBSERVATION

#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include "ct-fuzz-utils.h"

#define IDX_T TYPE(idx)
#define RUN_ID NS(run_idx)

typedef unsigned char IDX_T;
typedef uint32_t num_t;

extern IDX_T RUN_ID;

extern void NS(initialize_states)();
extern void NS(update_monitor_by_cond)(bool cond, char* fn, num_t ln, num_t cn);
extern void NS(update_monitor_by_addr)(char* addr, char* fn, num_t ln, num_t cn);
extern void NS(check_observations)();

#endif
