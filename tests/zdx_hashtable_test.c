#include <stdint.h>

#include "../zdx_util.h"

#define ZDX_SIMPLE_ARENA_IMPLEMENTATION
#include "../zdx_simple_arena.h"

typedef struct value {
  uint8_t age;
  char *university;
} val_t;

#define ZDX_HASHTABLE_IMPLEMENTATION
#define HT_MIN_CAPACITY 2
#define HT_VALUE_TYPE val_t
/* remove all the defines below this to default to using calloc(3) aka non-arena style allocator */
#define HT_ARENA_TYPE arena_t
#define HT_CALLOC arena_calloc
#define HT_FREE(...)
#include "../zdx_hashtable.h"


int main(void)
{
  ht_t ht = {0};
  arena_t arena = arena_create(8 KB);
  ht_ret_t ret = {0};

  {
    {
      ret = ht_set(&arena, &ht, "key-1", (val_t){ .age = 18, .university = "SOME UNI" });
      assertm(!ret.err, "Expected no error, Received: %s", ret.err);
      assertm(ht.length == 1, "Expected: 1, Received: %zu", ht.length);

      ret = ht_set(&arena, &ht, "key-2", (val_t){ .age = 28, .university = "SOME OTHER UNI" });
      assertm(!ret.err, "Expected no error, Received: %s", ret.err);
      assertm(ht.length == 2, "Expected: 2, Received: %zu", ht.length);

      ret = ht_set(&arena, &ht, "key-3", (val_t){ .age = 8, .university = NULL });
      assertm(!ret.err, "Expected no error, Received: %s", ret.err);
      assertm(ht.length == 3, "Expected: 3, Received: %zu", ht.length);

      ret = ht_set(&arena, &ht, "key-4", (val_t){ .age = 21, .university = "BNM" });
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


      log(L_INFO, "[HASHTABLE HAPPY PATH TESTS] OK!");

    }

    arena_reset(&arena);
    ht_reset(&ht);

    {
      ret = ht_get(&ht, "key-1");
      assertm(strcmp(ret.err, "Key not found") == 0, "Expected: Key not found, Received: %s", ret.err);

      log(L_INFO, "[HASHTABLE ERROR PATH TESTS] OK!");
    }
  }

  log(L_INFO, "<zdx_hashtable> All ok!");

  arena_free(&arena);
  ht_free(&ht);

  return 0;
}
