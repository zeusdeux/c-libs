# c-libs

[stb style](https://github.com/nothings/stb) single header libs for basic utilities in C programs.

## Usage

To use any library in this repository:

1. copy the header file into your project - for e.g., copy `zdx_da.h`
2. in *one* C file, do the following to include the implementation - for e.g., to use `zdx_da.h`
  ```c
  #define ZDX_DA_IMPLEMENTATION
  #include "<path to zdx_da.h>"
  ```
3. in all other files, just a `#include <path to lib header>` should suffice
4. done!

You can find usage examples in `tests/<lib>_test.c` - for e.g., `zdx_da.h` usage can be found in `tests/zdx_da_test.c`.

## Tests

```sh
# make test_<lib name> or make test_<lib name>_dbg for debug build of tests
make test_zdx_da
# OR
make test_zdx_da_dbg
```

Notes on how to make a single header-file library [can be found here](https://github.com/nothings/stb/blob/master/docs/stb_howto.txt).

## Libs

### `zdx_da.h`

This contains a (mostly) macro based dynamic array implementation.
It only works with `gcc` and `clang` for now as it uses `statement expressions`.

It requires a container `struct` with the following mandatory members:

- `size_t length`: will hold dynamic array length
- `size_t capacity`: will hold the capacity of the dynamic array as multiples of items
- `<type> *items`: this will hold all the items of type `<type>` pushed into the dynamic array. `<type>` can be anything you want.

Other members can also be specified in the `struct` alongside the above mandatory members.

For example, the following is a valid dynamic array container:

```c
typedef struct {
    char *name;
    uint8_t age;
} Person;

typedef struct {
    int i;
    size_t length;
    size_t capacity;
    Person *items;
    float f;
    union {
        uint8_t ui;
        int8_t si;
    }
} SomeStruct;
```

#### `#define`s

- `DA_ASSERT` - defaults to `assert()` from `<assert.h>`. Can be redefined if you'd like to not abort on assertion failure
- `DA_RESIZE_FACTOR` - defaults to 2. Decides by what factor the capactiy of the dynamic array is scaled. It can be redefined.
- `DA_MIN_CAPACITY` - defaults to 8. Can be redefined to control the minimum capacity of a dynamic array.

#### Macros

- `da_push(&container, ...elements)` - appends elements to `container.items` and returns the length after push
  ```c
  typedef struct {
      int member1;
      struct {
        union {
          int16_t nested_member1;
          float nester_member2;
        }
      } member2;
  } Item;

  typedef struct {
      size_t length;
      size_t capacity;
      Item *items;
      char *tag;
  } da_t;

  da_t container = {0};

  size_t len = da_push(&container, (Item){ .member1 = 80 }, (Item){ .member1 = 100, .member2.nested_member1 = "Hello!" });
  assert(len == 2 == container.length);
  ```
- `da_pop(&container)` - pops the last element in `container.items` and returns it
  ```c
  Item i = da_pop(&container);
  assert(i.member1 == 100);
  assert(container.length == 1);
  ```
- `da_free(&container)` - `free()`s `container.items`
  ```c
  da_free(&container);
  assert(container.length == 0);
  assert(container.capacity == 0);
  assert(container.items == NULL);
  ```

### `zdx_util`

This header contains utility macros and maybe utility functions in the future.

#### Enums

- `ZDX_LOG_LEVEL` - `L_ERROR` (1), `L_WARN` (2), `L_INFO` (3)

#### Macros

- `assertm(condition, ...)` - asserts if condition is truthy. If falsy, it prints the message provided in `...` to `stderr` and calls `abort()`
  ```c
  assertm(moreReplHistory.capacity == 2, "Expected: 2, Received: %zu", moreReplHistory.capacity);
  ```
- `log(ZDX_LOG_LEVEL, ...args)` - prints logs with given level to `stderr`.

  Logging can be disabled by flag `-DZDX_LOG_DISABLE`.
  ```c
  log(L_INFO, "%d tests passed. All ok!\n", 10);
  ```
- `dbg(...args)` - prints debug logs to `stderr`.

  It's disabled by default and can be enabled by `-DZDX_TRACE_ENABLE`.
  ```c
  dbg("Starting resizing: new capacity %zu", new_capacity);
  ```
- `bail(...args)` - prings `...args` to `stderr` and then calls `exit(1)`
  ```c
  bail("Oh no!");
  ```
