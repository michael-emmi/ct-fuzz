#include "ct-fuzz-observation.h"
#include "ct-fuzz-debug.h"

void NS(update_monitor_by_addr)(char* addr, char* fn, num_t ln, num_t cn) {
  if (START_OB) {
    START_OB = false;
    NS(dbg_print_addr)(addr, fn, ln, cn);
    NS(update_hash)((char*)&addr, sizeof(char*));
    START_OB = true;
  }
}
