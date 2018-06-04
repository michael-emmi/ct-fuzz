#ifndef CT_FUZZ_H
#define CT_FUZZ_H
void __ct_fuzz_public_in();

#define CT_FUZZ_SPEC(F) void __ct_fuzz_spec_ ## F

#endif
