#ifndef CT_FUZZ_OBSERVATION
#define CT_FUZZ_OBSERVATION

#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "xxhash.h"
#include "ct-fuzz-utils.h"
#include "ct-fuzz-states.h"
#include "ct-fuzz-debug.h"

#define STOP_SIGNAL 42

extern void PREFIX(update_monitor_by_cond)(bool cond);
extern void PREFIX(update_monitor_by_addr)(char* addr);
extern void PREFIX(check_observations)();

#endif
