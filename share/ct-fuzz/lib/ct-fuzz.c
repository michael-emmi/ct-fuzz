#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/wait.h>
#include "ct-fuzz-debug.h"
#include "ct-fuzz-states.h"
#include "ct-fuzz-observation.h"
#include "ct-fuzz-read.h"

#define PUBLIC_VALUE_MAX_COUNT 1000
//#define MALLOC_MAXIMUM_SIZE 4096
#define ASSUME_VIOLATION 240
//#define MEMORY_VIOLATION 420

#define ASSUME PREFIX(assume)

void ASSUME(bool cond, char* msg) {
  if (!cond) {
    if (msg)
      printf("%s\n", msg);
    exit(ASSUME_VIOLATION);
  }
}

void* PREFIX(malloc_wrapper)(size_t size) {
  //if (size > MALLOC_MAXIMUM_SIZE) {
  //  printf("dude, what the hell!\n");
  //  exit(MEMORY_VIOLATION);
  //} else
  return malloc(size);
}

void PREFIX(memcpy_wrapper)(char* dest, char* src, size_t num) {
  memcpy(dest, src, num);
}

void PREFIX(handle_public_value)(char* src, size_t size) {
  static char* PREFIX(public_values[PUBLIC_VALUE_MAX_COUNT]) = {NULL};
  static unsigned PREFIX(public_value_update_idx) = 0;
  static unsigned PREFIX(public_value_check_idx) = 0;

  if (!RUN_ID) {
    char* dest = (char*)PREFIX(malloc_wrapper)(size);
    PREFIX(public_values)[PREFIX(public_value_update_idx)++] = dest;
    PREFIX(memcpy_wrapper)(dest, src, size);
  } else {
    char* v = PREFIX(public_values)[PREFIX(public_value_check_idx)++];
    ASSUME(memcmp(src, v, size) == 0, "Public values mismatch.");
  }
}

void PREFIX(initialize)(void) {
  PREFIX(initialize_states)();
  PREFIX(dbg_init)();
  srand(time(0));
}

//void __ct_fuzz_switch(void) {
//  __ct_fuzz_run_idx += 1;
//}

void PREFIX(exec)(IDX_T);
void PREFIX(spec)(IDX_T);
void PREFIX(read_inputs)(void);
void PREFIX(merge_ptr_inputs)(void);

void PREFIX(main)(void) {
  PREFIX(initialize)();
  PREFIX(read_inputs)();

  // make this loop a macro? Yikes.
  for (RUN_ID = 0; RUN_ID < 2; ++RUN_ID)
    PREFIX(spec)(RUN_ID);

  PREFIX(merge_ptr_inputs)();

  for (RUN_ID = 0; RUN_ID < 2; ++RUN_ID) {
    pid_t pid = fork();
    if (pid == -1)
      exit(EXIT_FAILURE);
    else if (pid == 0) {
      // in the child process, good luck everybody else
      PREFIX(exec)(RUN_ID);
      exit(EXIT_SUCCESS);
    }
    else {
      int status;
      (void)waitpid(pid, &status, 0);
    }
  }

  PREFIX(check_observations)();
}

int main(void) {
  PREFIX(main)();
  return 0;
}
