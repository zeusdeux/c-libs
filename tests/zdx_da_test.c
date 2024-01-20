#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../zdx_da.h"
#include "../zdx_util.h"

typedef struct {
  char *input, *output;
} ReplHistoryItem;

typedef struct {
  int i;
  size_t length;
  size_t capacity;
  ReplHistoryItem *items;
} ReplHistory;

// Overridden for ease of asserting on .capacity member which depends on these #defines
#define DA_RESIZE_FACTOR 2
#define DA_MIN_CAPACITY 1

void print_repl_history(ReplHistory *r)
{
  printf("int i = %d\t| length %zu\t| capacity %zu\n", r->i, r->length, r->capacity);

  if (!r->length) {
    printf("No items!\n");
    return;
  }

  for (size_t i = 0; i < r->length; i++) {
    ReplHistoryItem rhi = r->items[i];
    printf("items[%zu] = { .input = %s, .output = %s }\n", i, rhi.input, rhi.output);
  }
}

int main(void)
{
  ReplHistory replHistory = {0};

  size_t idx = da_push(&replHistory,
          (ReplHistoryItem){ .input = "FIRST", .output = "ELEMENT" },
          (ReplHistoryItem){ .input = "console.log(\"omg\")", .output = "omg" },
          (ReplHistoryItem){ .input = "sin(90)", .output = "1" },
          (ReplHistoryItem){ .input = "typeof []", .output = "array" }
          );

  assert(replHistory.capacity == 4 && "dyn arr should grow to accomodate no., of items being pushed");
  assert(replHistory.length == 4 && "length should match no., of items pushed");
  assert(idx == replHistory.length && "return value of da_push should match length");

  replHistory.i = 200;
  assert(replHistory.i == 200 && "other members of struct being used as dyn arr should work as expected");

  assert(replHistory.capacity == replHistory.length);
  da_push(&replHistory, (ReplHistoryItem){ .input = "3 + 4", .output = "7" });

  assert(replHistory.capacity == 8 && "dyn arr should double in size (as DA_RESIZE_FACTOR is 2) when capacity is reached");

  idx = da_push(&replHistory, (ReplHistoryItem){ .input = "sizeof(int)", .output = "4" });
  da_push(&replHistory, (ReplHistoryItem){ .input = "sizeof(uint64_t)", .output = "8" });
  da_push(&replHistory, (ReplHistoryItem){ .input = "LAST", .output = "ELEMENT" });

  assert(replHistory.length == 8 && "length should match no., of items pushed");
  assert(idx == (replHistory.length - 2) && "return value of da_push should match length");
  assert(strcmp(replHistory.items[replHistory.length - 2].input, "sizeof(uint64_t)") == 0 && "element should match what was pushed");
  assert(strcmp(replHistory.items[replHistory.length - 2].output, "8") == 0 && "element should match what was pushed");

  replHistory.i -= 10;
  assert(replHistory.i == 190 && "other members of struct being used as dyn arr should work as expected");

  ReplHistoryItem ri = replHistory.items[5];
  assert(strcmp(ri.input, "sizeof(int)") == 0 && "direct access of item in dyn arr should work as expected");
  assert(strcmp(ri.output, "4") == 0 && "direct access of item in dyn arr should work as expected");
  assert(replHistory.length == 8 && "length should match no., of items pushed and remain unchanged on direct items access");
  assert(replHistory.capacity == 8 && "capacity should remain unchanged on direct items access");

  ReplHistoryItem rj = {
    .input = "SOME INPUT",
    .output = "SOME OUTPUT"
  };
  replHistory.items[5] = rj;
  ri = replHistory.items[5];

  assert(strcmp(ri.input, "SOME INPUT") == 0 && "direct access of item in dyn arr should work as expected");
  assert(strcmp(ri.output, "SOME OUTPUT") == 0 && "direct access of item in dyn arr should work as expected");
  assert(replHistory.length == 8 && "length should match no., of items pushed and remain unchanged on direct items access");
  assert(replHistory.capacity == 8 && "capacity should remain unchanged on direct items access");

  print_repl_history(&replHistory);

  ReplHistoryItem popped = da_pop(&replHistory);

  assert(replHistory.length == 7 && "length should reduce by one on da_pop()");
  assert(strcmp(popped.input, "LAST") == 0 && "popped element should match last element pushed");
  assert(strcmp(popped.output, "ELEMENT") == 0 && "popped element should match last element pushed");

  da_pop(&replHistory);
  da_pop(&replHistory);
  da_pop(&replHistory);
  da_pop(&replHistory);
  da_pop(&replHistory);
  da_pop(&replHistory);
  popped = da_pop(&replHistory);

  assert(replHistory.length == 0 && "length should be zero once all elements are popped");
  assert(strcmp(popped.input, "FIRST") == 0 && "last popped element should match first element pushed");
  assert(strcmp(popped.output, "ELEMENT") == 0 && "last popped element should match first element pushed");

  replHistory.i += 900;

  print_repl_history(&replHistory);

  da_free(&replHistory);

  print_repl_history(&replHistory);

  assert(replHistory.items == NULL && "After free(), items in dyn array container should be NULL ptr");
  assert(replHistory.length == 0 && "After free(), length in dyn array container should be NULL ptr");
  assert(replHistory.capacity == 0 && "After free(), capacity in dyn array container should be NULL ptr");

  assert(replHistory.i == 1090 && "After free(), other members of dyn array container should still work as expected");

  log(L_INFO, "<zdx_da_test> All ok!\n");

  return 0;
}
