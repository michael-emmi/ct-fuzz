# Annotations
## Specification Annotations
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
## Seed Generation Annotations
We use `CT_FUZZ_SEED` macro to define a seed generation function. Its syntax is pretty much the same with `CT_FUZZ_SPEC` except only types rather than types and identifiers show up in the argument list. For example, The seed generation definition of `foo` in the previous section is as follows,
```C
CT_FUZZ_SEED(void, foo, char*, short, long) {...}
```
We also provide a few macros to define seeds for the arguments of the function to test. They are listed below,
* `SEED_UNIT(T,N,...)` where `T` is the type of the seed while `N` is the name of the seed. The last argument is its initialization value.
* `SEED_1D_ARR(T,N,L,...)` where `T` is the type of the element in an array while `N`, again, is the same of the array. `L` is the length of the array. The last argument is an initializer of the array.
* `SEED_2D_ARR(T,N,L1,L2,...)` is similar to `SEED_1D_ARR`. The difference is that there are two length arguments.
* `SEED_3D_ARR(T,N,L1,L2,L3,...)` is similar to `SEED_2D_ARR` with an extra dimension.

Note that `SEED_2D_ARR` and `SEED_3D_ARR` can also be composed with combinations of `SEED_1D_ARR`.

Once the seeds of function arguments are defined, simply use `PRODUCE(F,...)` macro to generate one combined seed for the function. Macro `PRODUCE` simply creates an invocation of `F` with the seeds as arguments.

Take `foo` as example again. The body of its seed generation function would be,
```C
SEED_1D_ARR(char, A, 2, {'a','b'})
SEED_UNIT(short, B, 42)
// note that for primitive types, constants can be directly passed via PRODUCE
PRODUCE(foo, A, B, 24)
```
## Special Utility Functions
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
