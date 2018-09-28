#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/wait.h>
#include <time.h>
#include "ct-fuzz-debug.h"
#include "ct-fuzz-observation.h"
#include "ct-fuzz-read.h"

#define PUBLIC_VALUE_MAX_COUNT 1000
//#define MALLOC_MAXIMUM_SIZE 4096
#define ASSUME_VIOLATION 240
//#define MEMORY_VIOLATION 420

#define ASSUME NS(assume)

void ASSUME(bool cond, char* msg) {
  if (!cond) {
    if (msg)
      printf("%s\n", msg);
    exit(ASSUME_VIOLATION);
  }
}

void* NS(malloc_wrapper)(size_t size) {
  //if (size > MALLOC_MAXIMUM_SIZE) {
  //  printf("dude, what the hell!\n");
  //  exit(MEMORY_VIOLATION);
  //} else
  return malloc(size);
}

void NS(memcpy_wrapper)(char* dest, char* src, size_t num) {
  memcpy(dest, src, num);
}

void NS(handle_public_value)(char* src, size_t size) {
  static char* NS(public_values[PUBLIC_VALUE_MAX_COUNT]) = {NULL};
  static unsigned NS(public_value_update_idx) = 0;
  static unsigned NS(public_value_check_idx) = 0;

  if (!RUN_ID) {
    char* dest = (char*)NS(malloc_wrapper)(size);
    NS(public_values)[NS(public_value_update_idx)++] = dest;
    NS(memcpy_wrapper)(dest, src, size);
  } else {
    char* v = NS(public_values)[NS(public_value_check_idx)++];
    ASSUME(memcmp(src, v, size) == 0, "Public values mismatch.");
  }
}

void NS(initialize)(void) {
  NS(initialize_states)();
  NS(dbg_init)();
  srand(time(0));
  setbuf(stdout, NULL);
}

void NS(exec)(IDX_T);
void NS(spec)(IDX_T);
void NS(read_inputs)(void);
void NS(merge_ptr_inputs)(void);

void NS(main)(void) {
  START_OB = true;
  NS(initialize)();
  NS(read_inputs)();

  for (RUN_ID = 0; RUN_ID < 2; ++RUN_ID)
    NS(spec)(RUN_ID);

  //NS(merge_ptr_inputs)();

  for (RUN_ID = 0; RUN_ID < 2; ++RUN_ID) {
    pid_t pid = fork();
    if (pid == -1)
      exit(EXIT_FAILURE);
    else if (pid == 0) {
      NS(exec)(RUN_ID);
      exit(EXIT_SUCCESS);
    }
    else {
      int status;
      (void)waitpid(pid, &status, 0);
    }
  }

  NS(check_observations)();
}

int main(void) {
  NS(main)();
  return 0;
}
