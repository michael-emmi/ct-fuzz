#include "ct-fuzz-observation.h"
#include "ct-fuzz-debug.h"

#include <stdint.h>
#include <stdio.h>

#define CACHE_SIZE 32
#define ASSOCIATIVITY 512
#define BLOCK_BIT_SIZE 6 // shouldn't change
#define BLOCK_NUM 512
#define SET_BIT_SIZE 0
#define SET_NUM 1
#define TAG_BIT_SIZE 58
#define SET_INDEX_MASK 0x0
#define BLOCK_NUM_IN_SET 512

typedef struct {
  unsigned valid: 1;
  uint64_t tag: TAG_BIT_SIZE; 
  uint64_t rank;
} cache_line;

cache_line cache[BLOCK_NUM];
uint64_t rank[SET_NUM];

static bool NS(update_cache)(char* ptr) {
  uint64_t addr = (uint64_t)ptr;
  unsigned set_id = (addr >>= BLOCK_BIT_SIZE) & SET_INDEX_MASK;
  uint64_t tag = (addr >>= SET_BIT_SIZE);
  unsigned start_block = set_id * ASSOCIATIVITY;
  uint64_t time_stamp = rank[set_id]++;

  // check if there's a hit
  for (unsigned i = 0; i < BLOCK_NUM_IN_SET; ++i) { 
    cache_line* line = &cache[start_block+i];
    if (line->tag == tag && line->valid) {
      line->rank = time_stamp;
      if (DEBUG_ON)
        printf("hit: %lx, set: %u, block: %u\n", addr, set_id, start_block+i);
      return true;
    }
  }

  // ok, there's a miss 
  // first check if there are available slots
  for (unsigned i = 0; i < BLOCK_NUM_IN_SET; ++i) {
    cache_line* line = &cache[start_block+i];
    if (!line->valid) {
      line->valid = 1;
      line->tag = tag;
      line->rank = time_stamp;
      if (DEBUG_ON)
        printf("miss and empty: %lx, set: %u, block: %u\n", addr, set_id, start_block+i);
      return false;
    }
  }

  // time to replace
  cache_line* loser = &cache[start_block];
  for (unsigned i = 1; i < BLOCK_NUM_IN_SET; ++i) {
    cache_line* line = &cache[start_block+i];
    if (line->rank < loser->rank)
      loser = line;
  }
  // kick the loser out
  loser->rank = time_stamp;
  loser->tag = tag;
  if (DEBUG_ON)
    printf("miss and replace: %lx, set: %u, block: %u\n", addr, set_id, loser-cache);
  return false;
}

void NS(update_monitor_by_addr)(char* addr, char* fn, num_t ln, num_t cn) {
  if (START_OB) {
    START_OB = false;
    bool hit = NS(update_cache)(addr);
    NS(dbg_print_cache_access)(addr, hit, fn, ln, cn);
    NS(update_hash)((char*)&hit, sizeof(bool));
    START_OB = true;
  }
}
