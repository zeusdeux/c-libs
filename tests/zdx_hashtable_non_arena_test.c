#include <stdint.h>

#include "../zdx_util.h"

typedef struct value {
  uint8_t age;
  char *university;
} val_t;

#define ZDX_HASHTABLE_IMPLEMENTATION
#define HT_MIN_CAPACITY 2
#define HT_VALUE_TYPE val_t
#include "../zdx_hashtable.h"

void hashtable_non_arena_test(void)
{
  ht_t ht = {0};
  ht_ret_t ret = {0};

  {
    {
      ret = ht_set(&ht, "key-1", (val_t){ .age = 18, .university = "SOME UNI" });
      assertm(!ret.err, "Expected no error, Received: %s", ret.err);
      assertm(ht.length == 1, "Expected: 1, Received: %zu", ht.length);

      ret = ht_set(&ht, "key-2", (val_t){ .age = 28, .university = "SOME OTHER UNI" });
      assertm(!ret.err, "Expected no error, Received: %s", ret.err);
      assertm(ht.length == 2, "Expected: 2, Received: %zu", ht.length);

      ret = ht_set(&ht, "key-3", (val_t){ .age = 8, .university = NULL });
      assertm(!ret.err, "Expected no error, Received: %s", ret.err);
      assertm(ht.length == 3, "Expected: 3, Received: %zu", ht.length);

      ret = ht_set(&ht, "key-4", (val_t){ .age = 21, .university = "BNM" });
      assertm(!ret.err, "Expected no error, Received: %s", ret.err);
      assertm(ht.length == 4, "Expected: 4, Received: %zu", ht.length);

      ret = ht_get(&ht, "key-1");
      assertm(!ret.err, "Expected no error, Received: %s", ret.err);
      assertm(ret.value.age == 18, "Expected: 18, Received: %hu", ret.value.age);
      assertm(strcmp(ret.value.university, "SOME UNI") == 0, "Expected: \"SOME UNI\", Received: %s", ret.value.university);
      assertm(ht.length == 4, "Expected: 4, Received: %zu", ht.length);

      ret = ht_remove(&ht, "key-1");
      assertm(!ret.err, "Expected no error, Received: %s", ret.err);
      assertm(ret.value.age == 18, "Expected: 18, Received: %hu", ret.value.age);
      assertm(strcmp(ret.value.university, "SOME UNI") == 0, "Expected: \"SOME UNI\", Received: %s", ret.value.university);
      assertm(ht.length == 3, "Expected: 3, Received: %zu", ht.length);

      log(L_INFO, "[HASHTABLE NON-ARENA HAPPY PATH TESTS] OK!");
    }

    ht_reset(&ht);
    assertm(ht.length == 0, "Expected: 0, Received: %zu", ht.length);
    for (size_t i = 0; i < ht.capacity; i++) {
      assertm(!ht.items[i].occupied, "Expected: false, Received: %s", ht.items[i].occupied ? "true" : "false");
    }

    {
      ret = ht_get(&ht, "key-1");
      assertm(strcmp(ret.err, "Key not found") == 0, "Expected: Key not found, Received: %s", ret.err);

      log(L_INFO, "[HASHTABLE NON-ARENA ERROR PATH TESTS] OK!");
    }
  }

  ht_free(&ht);
  assertm(ht.items == NULL, "Expected: NULL, Received: %p", (void *)ht.items);
  assertm(ht.length == 0, "Expected: 0, Received: %zu", ht.length);
  assertm(ht.capacity == 0, "Expected: 0, Received: %zu", ht.capacity);

  log(L_INFO, "<zdx_hashtable_non_arena_test> All ok!");
}
