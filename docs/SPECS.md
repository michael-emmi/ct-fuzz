# Annotations

We use `SPEC` macro to annotate a function. Annotation function must have exact the same arguments as the function being annotated. For example,
```C
// source function
void foo(char* a, short b, long c);

// specification function
SPEC(foo)(char* a, short b, long c);
```

Note that `SPEC` functions should not have any side-effects. For example, the following function annotation should be avoided.
```C
SPEC(foo)(int a) {
  for (unsigned i = 0; i < a; ++i)
    // g is a global
    g++;
  __ct_fuzz_public_in(g % 2);
}
```

We use `__ct_fuzz_public_in` function to annotate public information. Arguments of pointer types are assumed to be public. However, the elements they point to are not necessarily made public. To annotate public pointer elements, call function `__ct_fuzz_public_in` with two arguments, the first argument being the pointer and the second being the number of pointer elements. For example,
```C
SPEC(foo)(char* a) {
  __ct_fuzz_public_in(a, strlen(a));
}
```


Function `__ct_fuzz_array_len` returns the number of elements a pointer argument points to. A pointer to a single element should have length 0.

Macro `CT_FUZZ_ASSUME` is used to place assumptions. If the condition argument doesn't hold, the execution stops.
