#include <stdint.h>

#include "../zdx_test_utils.h"

#ifdef HT_ARENA_TYPE
#define ZDX_SIMPLE_ARENA_IMPLEMENTATION
#include "../zdx_simple_arena.h"
#endif // HT_ARENA_TYPE

typedef struct value {
  uint8_t age;
  char *university;
} val_t;

#define ZDX_HASHTABLE_IMPLEMENTATION
#define HT_MIN_CAPACITY 2
#define HT_VALUE_TYPE val_t
#include "../zdx_hashtable.h"

int main(void)
{
  TEST_PROLOGUE;

  ht_t ht = {0};
  ht_ret_t ret = {0};
#ifdef HT_ARENA_TYPE
  arena_t arena = arena_create(8 KB);
#endif // HT_ARENA_TYPE

  {
    /* happy paths */
    {
#ifdef HT_ARENA_TYPE
      ret = ht_set(&arena, &ht, "key-1", (val_t){ .age = 18, .university = "SOME UNI" });
#else
      ret = ht_set(&ht, "key-1", (val_t){ .age = 18, .university = "SOME UNI" });
#endif // HT_ARENA_TYPE
      assertm(!ret.err, "Expected no error, Received: %s", ret.err);
      assertm(ht.length == 1, "Expected: 1, Received: %zu", ht.length);

#ifdef HT_ARENA_TYPE
      ret = ht_set(&arena, &ht, "key-2", (val_t){ .age = 28, .university = "SOME OTHER UNI" });
#else
      ret = ht_set(&ht, "key-2", (val_t){ .age = 28, .university = "SOME OTHER UNI" });
#endif // HT_ARENA_TYPE
      assertm(!ret.err, "Expected no error, Received: %s", ret.err);
      assertm(ht.length == 2, "Expected: 2, Received: %zu", ht.length);

#ifdef HT_ARENA_TYPE
      ret = ht_set(&arena, &ht, "key-3", (val_t){ .age = 8, .university = NULL });
#else
      ret = ht_set(&ht, "key-3", (val_t){ .age = 8, .university = NULL });
#endif // HT_ARENA_TYPE
      assertm(!ret.err, "Expected no error, Received: %s", ret.err);
      assertm(ht.length == 3, "Expected: 3, Received: %zu", ht.length);

#ifdef HT_ARENA_TYPE
      ret = ht_set(&arena, &ht, "key-4", (val_t){ .age = 21, .university = "BNM" });
#else
      ret = ht_set(&ht, "key-4", (val_t){ .age = 21, .university = "BNM" });
#endif // HT_ARENA_TYPE
      assertm(!ret.err, "Expected no error, Received: %s", ret.err);
      assertm(ht.length == 4, "Expected: 4, Received: %zu", ht.length);

      ret = ht_get(&ht, "key-1");
      assertm(!ret.err, "Expected no error, Received: %s", ret.err);
      assertm(ret.value.age == 18, "Expected: 18, Received: %hu", ret.value.age);
      assertm(strcmp(ret.value.university, "SOME UNI") == 0, "Expected: \"SOME UNI\", Received: %s", ret.value.university);
      assertm(ht.length == 4, "Expected: 4, Received: %zu", ht.length);

#if defined(HT_AUTO_SHRINK) && defined(HT_ARENA_TYPE)
      ret = ht_remove(&arena, &ht, "key-1");
#else
      ret = ht_remove(&ht, "key-1");
#endif // HT_ARENA_TYPE && HT_ARENA_TYPE
      assertm(!ret.err, "Expected no error, Received: %s", ret.err);
      assertm(ret.value.age == 18, "Expected: 18, Received: %hu", ret.value.age);
      assertm(strcmp(ret.value.university, "SOME UNI") == 0, "Expected: \"SOME UNI\", Received: %s", ret.value.university);
      assertm(ht.length == 3, "Expected: 3, Received: %zu", ht.length);
    }

    ht_reset(&ht);
    assertm(ht.length == 0, "Expected: 0, Received: %zu", ht.length);
    for (size_t i = 0; i < ht.capacity; i++) {
      assertm(!ht.items[i].occupied, "Expected: false, Received: %s", ht.items[i].occupied ? "true" : "false");
    }

#ifdef HT_ARENA_TYPE
    arena_reset(&arena);
#endif // HT_ARENA_TYPE

    /* error paths */
    {
      ret = ht_get(&ht, "key-1");
      assertm(strcmp(ret.err, "Key not found (empty hashtable)") == 0, "Expected: Key not found, Received: %s", ret.err);

    }
  }

  ht_free(&ht);
  assertm(ht.items == NULL, "Expected: NULL, Received: %p", (void *)ht.items);
  assertm(ht.length == 0, "Expected: 0, Received: %zu", ht.length);
  assertm(ht.capacity == 0, "Expected: 0, Received: %zu", ht.capacity);

#ifdef HT_ARENA_TYPE
  arena_free(&arena);
#endif // HT_ARENA_TYPE

  TEST_EPILOGUE;

  return 0;
}
