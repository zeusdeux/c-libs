#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
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

#pragma GCC diagnostic ignored "-Wmacro-redefined"

// Overridden for ease of asserting on .capacity member which depends on these #defines
#define DA_RESIZE_FACTOR 2
#define DA_MIN_CAPACITY 1

void print_repl_history(ReplHistory *r)
{
  const char *zdx_disable_test_output = getenv("ZDX_DISABLE_TEST_OUTPUT");
  bool disable_test_output = zdx_disable_test_output != NULL && ((strcmp(zdx_disable_test_output, "1") == 0)
                                                                 || (strcmp(zdx_disable_test_output, "true") == 0)
                                                                 || (strcmp(zdx_disable_test_output, "TRUE") == 0));

  if (disable_test_output) {
    return;
  }

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
  ReplHistory moreReplHistory = {0};
  ReplHistoryItem tempItem = { .input = "TEMP", .output = "ITEM" };

  /* MAYBE WRITE TESTS FOR THESE USING DejaGnu: https://www.embecosm.com/appnotes/ean8/ean8-howto-dejagnu-1.0.pdf */
  /* UNCOMMENT AND CHECK THE FOLLOWING THROW COMPILE ERRORS */
  /* da_push(NULL, NULL); */
  /* da_push(NULL, tempItem); */
  /* da_push(&moreReplHistory, NULL); */
  /* da_push(&moreReplHistory, tempItem, NULL); */
  /* da_push(&moreReplHistory, NULL, tempItem); */
  /* da_push(&moreReplHistory, *((ReplHistoryItem *)NULL), tempItem); // requires -Werror=null-dereference to catch during compile */

  /* UNCOMMENT AND CHECK THE FOLLOWING THROW RUNTIME ASSERTION ERRORS */
  /* ReplHistory *r = &moreReplHistory; */
  /* ReplHistoryItem *ti = &tempItem; */
  /* r = NULL; */
  /* ti = NULL; */
  /* da_push(r, tempItem); */
  // I have no idea how to catch this during compile or runtime when
  // compiled using -O3 as it seems to be removed by the compiler.
  // Segfaults in debug build though.
  /* da_push(&moreReplHistory, *ti); */
  /* END */

  da_push(&moreReplHistory, tempItem);
  da_push(&moreReplHistory, tempItem);

  assertm(moreReplHistory.capacity == 2, "Expected: 2, Received: %zu", moreReplHistory.capacity);

  tempItem.input = "CHANGED";
  moreReplHistory.items[1].input = "CHANGED AS WELL";
  assertm(strcmp(tempItem.input, "CHANGED") == 0, "Expected: \"CHANGED\", Received: \"%s\"", tempItem.input);
  assertm(strcmp(moreReplHistory.items[1].input, "CHANGED AS WELL") == 0, "Expected: \"CHANGED AS WELL\", Received: \"%s\"", tempItem.input);
  assertm(tempItem.input != moreReplHistory.items[1].input, "Expected: true, Received: false");
  assertm(moreReplHistory.items[0].input != moreReplHistory.items[1].input, "Expected: true, Received: false");
  da_deinit(&moreReplHistory);

  size_t idx = da_push(&replHistory,
                       (ReplHistoryItem){ .input = "FIRST", .output = "ELEMENT" },
                       (ReplHistoryItem){ .input = "console.log(\"omg\")", .output = "omg" },
                       (ReplHistoryItem){ .input = "sin(90)", .output = "1" },
                       (ReplHistoryItem){ .input = "typeof []", .output = "array" }
                       );

  assertm(replHistory.capacity == 4, "dyn arr should grow to accomodate no., of items being pushed");
  assertm(replHistory.length == 4, "length should match no., of items pushed");
  assertm(idx == replHistory.length, "return value of da_push should match length");

  replHistory.i = 200;
  assertm(replHistory.i == 200, "other members of struct being used as dyn arr should work as expected");

  assertm(replHistory.capacity == replHistory.length,
          "Expected capacity and length to be equal. Received: capacity %zu, length %zu",
          replHistory.capacity, replHistory.length);
  da_push(&replHistory, (ReplHistoryItem){ .input = "3 + 4", .output = "7" });

  assertm(replHistory.capacity == 8, "dyn arr should double in size (as DA_RESIZE_FACTOR is 2) when capacity is reached");

  idx = da_push(&replHistory, (ReplHistoryItem){ .input = "sizeof(int)", .output = "4" });
  da_push(&replHistory, (ReplHistoryItem){ .input = "sizeof(uint64_t)", .output = "8" });
  da_push(&replHistory, (ReplHistoryItem){ .input = "LAST", .output = "ELEMENT" });

  assertm(replHistory.length == 8, "length should match no., of items pushed");
  assertm(idx == (replHistory.length - 2), "return value of da_push should match length");
  assertm(strcmp(replHistory.items[replHistory.length - 2].input, "sizeof(uint64_t)") == 0, "element should match what was pushed");
  assertm(strcmp(replHistory.items[replHistory.length - 2].output, "8") == 0, "element should match what was pushed");

  replHistory.i -= 10;
  assertm(replHistory.i == 190, "other members of struct being used as dyn arr should work as expected. Expected: 190, Received: %d", replHistory.i);

  ReplHistoryItem ri = replHistory.items[5];
  assertm(strcmp(ri.input, "sizeof(int)") == 0, "direct access of item in dyn arr should work as expected");
  assertm(strcmp(ri.output, "4") == 0, "direct access of item in dyn arr should work as expected");
  assertm(replHistory.length == 8, "length should match no., of items pushed and remain unchanged on direct items access");
  assertm(replHistory.capacity == 8, "capacity should remain unchanged on direct items access");

  ReplHistoryItem rj = {
    .input = "SOME INPUT",
    .output = "SOME OUTPUT"
  };
  replHistory.items[5] = rj;
  ri = replHistory.items[5];

  assertm(strcmp(ri.input, "SOME INPUT") == 0, "direct access of item in dyn arr should work as expected");
  assertm(strcmp(ri.output, "SOME OUTPUT") == 0, "direct access of item in dyn arr should work as expected");
  assertm(replHistory.length == 8, "length should match no., of items pushed and remain unchanged on direct items access");
  assertm(replHistory.capacity == 8, "capacity should remain unchanged on direct items access");

  print_repl_history(&replHistory);

  ReplHistoryItem popped = da_pop(&replHistory);

  assertm(replHistory.length == 7, "length should reduce by one on da_pop()");
  assertm(strcmp(popped.input, "LAST") == 0, "popped element should match last element pushed");
  assertm(strcmp(popped.output, "ELEMENT") == 0, "popped element should match last element pushed");

  da_pop(&replHistory);
  da_pop(&replHistory);
  da_pop(&replHistory);
  da_pop(&replHistory);
  da_pop(&replHistory);
  da_pop(&replHistory);
  popped = da_pop(&replHistory);

  assertm(replHistory.length == 0, "length should be zero once all elements are popped");
  assertm(strcmp(popped.input, "FIRST") == 0, "last popped element should match first element pushed");
  assertm(strcmp(popped.output, "ELEMENT") == 0, "last popped element should match first element pushed");

  replHistory.i += 900;

  print_repl_history(&replHistory);

  da_deinit(&replHistory);

  print_repl_history(&replHistory);

  assertm(replHistory.items == NULL, "After free(), items in dyn array container should be NULL ptr");
  assertm(replHistory.length == 0, "After free(), length in dyn array container should be 0");
  assertm(replHistory.capacity == 0, "After free(), capacity in dyn array container should be 0");

  assertm(replHistory.i == 1090, "After free(), other members of dyn array container should still work as expected");

  log(L_INFO, "<zdx_da_test> All ok!\n");

  return 0;
}
