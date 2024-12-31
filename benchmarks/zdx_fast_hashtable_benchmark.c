#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// we want to use assertm for test like asserts so we
// enable assertm by undef-ing NDEBUG if it's defined
#ifdef NDEBUG
#undef NDEBUG
#include "../zdx_util.h"
#endif

typedef struct {
  char *val;
} my_type_t;

#define ZDX_FAST_HASHTABLE_IMPLEMENTATION
#define FHT_VALUE_TYPE my_type_t
/* #define FHT_MAX_KEYLEN 8 */
#include "../zdx_fast_hashtable.h"

void run(const uint32_t insert_count, const uint32_t lookup_count, const uint8_t max_key_len)
{
  fht_t fht = fht_init(insert_count);

  printf("\n-------------------------------------------INFO-------------------------------------------\n");
  printf("Table: Fast hashtable, Max key length: %u, Unique Inserts: %u, Random Lookups: %u\n", max_key_len, insert_count, lookup_count);
  printf("------------------------------------------------------------------------------------------\n");

  srand(1337);

  PROF_START(INSERTS);
  for (uint32_t i = 0; i < insert_count; i++) {
    uint32_t len = (uint32_t)rand() % (max_key_len + 1);
    len = len < 4 ? 7 : len;

    char key[len + 1];

    for (size_t i = 0; i < len; i++) {
        int c1 = rand() % 126;
        c1 = c1 < 33 ? (33 + ((size_t)rand()) % 93) : c1;
        key[i] = c1;
      }
    key[len] = 0; // we set this NUL byte as we use strlen(key) in the call to fht_set() below

    char *val_str = malloc(sizeof(*val_str) * len);
    memcpy(val_str, key, len);
    my_type_t val = {.val = val_str};
    uint8_t key_len = strlen(key);

    log(L_INFO, "Set key `%s` (len = %u) as %s", key, key_len, val.val);

    const fht_add_ret_val_t add_ret_val = fht_add(&fht, key, key_len, val);

    if (add_ret_val.err) {
      log(L_ERROR, "Error: Failed to set key `%s` due to `%s`", key, fht_err_str(add_ret_val.err));
      exit(1);
    }
  }
  PROF_END(INSERTS);

  PROF_START(LOOKUPS);
  for (uint32_t i = 0; i < lookup_count; i++) {
    // mimic random access of hashtable
    uint32_t random_key_index = (uint32_t)rand() % insert_count;
    fht_key_t *key_obj = &fht.keys[random_key_index];
    char *key = key_obj->key;
    uint8_t key_len = key_obj->key_len;

    const fht_get_ret_val_t get_ret_val = fht_get(&fht, key, key_len);

    if (get_ret_val.err) {
      log(L_ERROR, "Error: Failed to get key `%s` due to `%s`", key, fht_err_str(get_ret_val.err));
      exit(1);
    }

    log(L_INFO, "Got key `%s` (len = %u) as `%s`", key, key_len, get_ret_val.val.val);

    assertm(memcmp(get_ret_val.val.val, key, key_len) == 0,
            "Expected: `%s` as val (key = `%s`), Received: `%s` as val", key, key, get_ret_val.val.val);
  }
  PROF_END(LOOKUPS);

  assertm(fht.count == insert_count, "Expected: %u, Received: %u", insert_count, fht.count);

  fht_deinit(&fht);
}

int main(void)
{
  printf("\n-------------------------------------------HEADER-----------------------------------------\n");
  printf("sizeof(fht_key_t): %zu bytes\n", sizeof(fht_key_t));
  printf("sizeof(fht_value_t): %zu bytes\n", sizeof(fht_value_t));
  printf("sizeof(fht_t): %zu bytes\n", sizeof(fht_t));
  printf("sizeof(fht_key_status_t): %zu bytes\n", sizeof(fht_key_status_t));
  printf("sizeof(fht_get_ret_val_t): %zu bytes\n", sizeof(fht_get_ret_val_t));
  printf("sizeof(fht_add_ret_val_t): %zu bytes\n", sizeof(fht_add_ret_val_t));
  printf("------------------------------------------------------------------------------------------\n");

  run(1e1, 3e7, FHT_MAX_KEYLEN);
  run(1e2, 2e7+5e6, FHT_MAX_KEYLEN);
  run(1e3, 1e7+8e6+5e5, FHT_MAX_KEYLEN);
  run(1e4, 1e7+1e6+7e5+2e4, FHT_MAX_KEYLEN);
  run(1e5, 3e6+2e5, FHT_MAX_KEYLEN);
  run(1e6, 3e5, FHT_MAX_KEYLEN);
  printf("\nDone!\n");
}
