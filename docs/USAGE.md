# Annotations

We use `CT_FUZZ_SPEC` macro to annotate a function. Annotation macro starts with the return type of a function, then the function name, followed by function arguments. For example,
```C
// source function
void foo(char* a, short b, long c);

// specification definition
CT_FUZZ_SPEC(void, foo, char* a, short b, long c) {...}
```

Note that the body of `CT_FUZZ_SPEC` should not have any side-effects. For example, the following function annotation should be avoided.
```C
CT_FUZZ_SPEC(void, foo, int a) {
  for (unsigned i = 0; i < a; ++i)
    // g is a global
    g++;
  __ct_fuzz_public_in(g % 2);
}
```

We use `__ct_fuzz_public_in` function to annotate public information. Arguments of pointer types are assumed to be public. However, the elements they point to are not necessarily made public. To annotate public pointer elements, call function `__ct_fuzz_public_in` with two arguments, the first argument being the pointer and the second being the number of pointer elements. For example,
```C
CT_FUZZ_SPEC(void, foo, char* a) {
  __ct_fuzz_public_in(a, strlen(a));
}
```


Function `__ct_fuzz_array_len` returns the number of elements a pointer argument points to. A pointer to a single element should have length 1.

Macro `CT_FUZZ_ASSUME` is used to place assumptions. If the condition argument doesn't hold, the execution stops.

# Run ct-fuzz
There are currently two modes for `ct-fuzz` depending on the types of input files.
## Monolithic input files
If the input file given to `ct-fuzz` contains both specification functions as well as source functions to fuzz, simply pass input file name to `ct-fuzz`. For example,

```
ct-fuzz sort_negative.c --entry-point=sort3
```
## Separate specification and source
If the specification functions and source functions are in different files, we expect the users to compile source functions to object files using `ct-fuzz-afl-clang-fast*` and then invoke `ct-fuzz` by adding `--obj-file <file>` flag. For example,

```
ct-fuzz-afl-clang-fast sort_negative.c -c -O2
ct-fuzz sort_negative_spec.c --entry-point=sort3 --obj-file=sort_negative.o
```
