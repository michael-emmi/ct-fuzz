#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/wait.h>

#define PUBLIC_VALUE_MAX_COUNT 1000

typedef unsigned char idx_t;

void __ct_fuzz_assume(bool cond, char* msg) {
  if (!cond) {
    if (msg)
      printf("%s\n", msg);
    exit(1);
  }
}

size_t __ct_fuzz_size_t_max(size_t a, size_t b){
  if (a > b)
    return a;
  else
    return b;
}

//idx_t __ct_fuzz_run_idx;

void __ct_fuzz_handle_public_value(char* src, size_t size, idx_t idx) {
  static char* __ct_fuzz_public_values[PUBLIC_VALUE_MAX_COUNT] = {NULL};
  static unsigned __ct_fuzz_public_value_update_idx = 0;
  static unsigned __ct_fuzz_public_value_check_idx = 0;

  if (!idx) {
    char* dest = (char*)malloc(size);
    __ct_fuzz_public_values[__ct_fuzz_public_value_update_idx++] = dest;
    memcpy(dest, src, size);
  } else {
    char* v = __ct_fuzz_public_values[__ct_fuzz_public_value_check_idx++];
    __ct_fuzz_assume(memcmp(src, v, size) == 0, "Public values mismatch.");
  }
}

void __ct_fuzz_stdin_read(void* buf, size_t size) {
  read(0, buf, size);
}

void* create_shared_memory(size_t size) {
  return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
}

void __ct_fuzz_initialize(void) {
  // nothing to do
}

//void __ct_fuzz_switch(void) {
//  __ct_fuzz_run_idx += 1;
//}

void __ct_fuzz_read_inputs(void);
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

void __ct_fuzz_exec(idx_t);
/* supposed body
 * read input from pipe
 * call function in test
 */

void __ct_fuzz_spec(idx_t);

void __ct_fuzz_check_observations() {
  // TODO
}

void __ct_fuzz_main(void) {
  __ct_fuzz_initialize();
  __ct_fuzz_read_inputs();

  for (idx_t i = 0; i < 2; ++i)
    __ct_fuzz_spec(i);

  for (idx_t i = 0; i < 2; ++i) {
    pid_t pid = fork();
    if (pid == -1)
      exit(EXIT_FAILURE);
    else if (pid == 0) {
      // in the child process, good luck everybody else
      __ct_fuzz_exec(i);
      exit(EXIT_SUCCESS);
    }
    else {
      int status;
      (void)waitpid(pid, &status, 0);
    }
  }

  __ct_fuzz_check_observations();
}

int main(void) {
  __ct_fuzz_main();
  return 0;
}
