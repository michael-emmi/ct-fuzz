#ifndef CT_FUZZ_OBSERVATION
#define CT_FUZZ_OBSERVATION

#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>
#include "xxhash.h"
#include "ct-fuzz-utils.h"
#include "ct-fuzz-debug.h"

#define STOP_SIGNAL 42

extern void NS(update_monitor_by_cond)(bool cond);
extern void NS(update_monitor_by_addr)(char* addr);
extern void NS(check_observations)();

#endif
