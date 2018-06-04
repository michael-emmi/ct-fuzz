#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>

#define PUBLIC_VALUE_MAX_COUNT 1000
char* __ct_fuzz_public_values[PUBLIC_VALUE_MAX_COUNT];

void __ct_fuzz_assume(bool cond, char* msg) {
  if (!cond) {
    if (msg)
      printf("%s\n", msg);
    exit(1);
  }
}

unsigned char* __ct_fuzz_run_idx;

bool __ct_fuzz_input_check = false;

bool __ct_fuzz_is_first_run(void) {
  return *__ct_fuzz_run_idx == 0U;
}

bool __ct_fuzz_reset(void) {
  return *__ct_fuzz_run_idx == 0;
}

void __ct_fuzz_input_check_start(void) {
  __ct_fuzz_input_check = true;
}

void __ct_fuzz_handle_public_value(char* src, size_t size) {
  static char* __ct_fuzz_public_values[PUBLIC_VALUE_MAX_COUNT] = {NULL};
  static unsigned __ct_fuzz_public_value_update_idx = 0;
  static unsigned __ct_fuzz_public_value_check_idx = 0;

  if (!__ct_fuzz_input_check) {
    char* dest = (char*)malloc(size);
    __ct_fuzz_public_values[__ct_fuzz_public_value_update_idx++] = dest;
    memcpy(dest, src, size);
  } else {
    char* v = __ct_fuzz_public_values[__ct_fuzz_public_value_check_idx++];
    __ct_fuzz_assume(memcmp(src, v, size) == 0, "Public values mismatch.");
  }
}

int __ct_fuzz_input_pipe[2];

void __ct_fuzz_input_pipe_write(void* buf, size_t size) {
  write(__ct_fuzz_input_pipe[1], buf, size);
}

void __ct_fuzz_input_pipe_read(void* buf, size_t size) {
  read(__ct_fuzz_input_pipe[0], buf, size);
}

void __ct_fuzz_stdin_read(void* buf, size_t size) {
  read(0, buf, size);
}

void* create_shared_memory(size_t size) {
  return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
}

void __ct_fuzz_initialize(void) {
  // initialize shared data among processes
  __ct_fuzz_run_idx = (unsigned char*)create_shared_memory(sizeof(unsigned char*));
  *__ct_fuzz_run_idx = 0U;
  // initialize a pipe to transfer input data
  pipe(__ct_fuzz_input_pipe);
}

void __ct_fuzz_switch(void) {
  *__ct_fuzz_run_idx += 1;
}

void __ct_fuzz_read_and_check_inputs(void);
/* supposed body
 * {
 *  read pub_input_i
 *  read private_input_i
 *  ...
 *  read pub_input_i'
 *  read private_input_i'
 *  check pub_input_i, pub_input_i' (call spec)
 *  ...
 *
 *  write public_input_i into pipe
 *  write private_input_i into pipe
 *  ...
 *  write public_input_i' into pipe
 *  write private_input_i' into pipe
 *
 * }
 */

void __ct_fuzz_exec(void);
/* supposed body
 * read input from pipe
 * call function in test
 */

void __ct_fuzz_check_observations() {}

void __ct_fuzz_main(void) {
  static unsigned char counter = 0;
  __ct_fuzz_initialize();
  __ct_fuzz_read_and_check_inputs();
  do {
    pid_t pid = fork();
    if (pid == -1)
      exit(EXIT_FAILURE);
    else if (pid == 0) {
      // in the child process, good luck everybody else
      __ct_fuzz_exec();
      exit(EXIT_SUCCESS);
    }
    else {
      int status;
      (void)waitpid(pid, &status, 0);
      __ct_fuzz_switch();
      counter++;
    }
  } while(counter < 2);
  __ct_fuzz_check_observations();
}

int main(void) {
  __ct_fuzz_main();
  return 0;
}
