// RUN: ct-fuzz %s --entry-point=foo -o %t --seed-file %t1
// RUN: seq 2 | xargs -i -- cat %t1 | xargs -0 echo -n -e | tr -d '\n' | %t > %t2
// RUN: diff %t2 %s.output
#include <stdio.h>
#include "ct-fuzz.h"

#define LEN 2

struct T {
  short i;
  int* p;
};

struct S {
  char a;
  int* b;
  int** c;
  short d[LEN];
  struct T e;
  struct T* f;
};

void foo(struct S* s) {
  s->a = 1;
}

CT_FUZZ_SPEC(void, foo, struct S* s) {
  printf("%d,", s->a);
  for (unsigned short i = 0; i < __ct_fuzz_get_arr_len(s->b); ++i)
    printf("%d,", s->b[i]);
  for (unsigned short i = 0; i < __ct_fuzz_get_arr_len(s->c); ++i)
    for (unsigned short j = 0; j < __ct_fuzz_get_arr_len(*s->c); ++j)
      printf("%d,", s->c[i][j]);
  for (unsigned short i = 0; i < LEN; ++i)
    printf("%d,", s->d[i]);
  printf("%d,", s->e.i);
  for (unsigned short i = 0; i < __ct_fuzz_get_arr_len(s->e.p); ++i)
    printf("%d,", s->e.p[i]);
  printf("%d,", s->f->i);
  for (unsigned short i = 0; i < __ct_fuzz_get_arr_len(s->f->p); ++i)
    printf("%d,", s->f->p[i]);
  printf("\n");
}

CT_FUZZ_SEED(void, foo, struct S*) {
  SEED_1D_ARR(int, i, LEN, {2,1})
  SEED_2D_ARR(int, j, LEN, LEN, {{5, 6},{7,8}})
  SEED_1D_ARR(int, k, LEN, {31,32})
  SEED_UNIT(struct T, t, {0,k})
  SEED_UNIT(struct S, s, {1,i,j,{0,40},{0,k},&t})
  PRODUCE(foo, &s)
}

