#ifndef ZDX_FAST_HASHTABLE_H_
#define ZDX_FAST_HASHTABLE_H_

#include <stdint.h>
#include <assert.h>

#if !defined(FHT_VALUE_TYPE)
static_assert(0, "FHT_VALUE_TYPE must be defined to use zdx_fast_hashtable.h");
#endif

#ifndef FHT_API
#define FHT_API
#endif

#ifndef FHT_ASSERT
#include "zdx_util.h"
#define FHT_ASSERT assertm
#endif

// We only support ASCII values as keys
typedef struct zdx_fast_hashtable_key {
  // 3 bits are unused but kept to ensure bit fields add up to 32bits aka 4bytes
  // to not rely on compiler introduced padding to ensure that this struct is 12bytes total!
  unsigned int used : 3;

  // 24 bits represent the index of value in the values array
  //   Max count of key/value pairs this hashtable can store therefore is 2^24 - 1 which is 16_777_215
  unsigned int val_index : 24;

  // 5 bits represent the length of the key
  //   Max key length is therefore 2^5 - 1 which is 31 but we allow only a max key length of 16
  unsigned int key_len : 5;

  // 4 bytes that hold first 4 bytes of the key
  char key_prefix[4];

  union fht_key_rest_t {
    // If key length is less than 8 then the suffix is stored here
    // this storing the full key directly in this zdx_fast_hashtable_key struct
    // NOTE: key_suffix is kept right after prefix to make sure the bytes are contiguous
    //       for keys that are between length 5 and 8
    char key_suffix[4];
    // If the key length is greater than 8, then we store the suffix in an
    // array that holds values of type fht_key_suffix_t and store the index here.
    // Even though this is 32bits, we only use 24 bits as that's the maximum number
    // of values we can store and therefore that's the max number of keys we can store
    // and hence the max number of key suffixes if every key has length greater than 8
    uint32_t key_suffix_index;
  } rest;
} fht_key_t;

typedef struct zdx_fast_hashtable_key_suffix {
  char suffix[12];
} fht_key_suffix_t;

typedef struct zdx_fast_hashtable_value {
  FHT_VALUE_TYPE val;
} fht_value_t;

typedef struct zdx_fast_hashtable {
    uint32_t cap;
    uint32_t count;
    fht_key_t *keys;
    fht_value_t *values;
  /* struct fht_key_suffixes_t { */
  /*   uint32_t count; */
  /*   uint32_t cap; */
  /*   fht_key_suffix_t *items; */
  /* } key_suffixes; */
} fht_t;

typedef enum zdx_fast_hashtable_error {
  FHT_ERR_NONE = 0,
  FHT_ERR_KEY_NOT_FOUND,
  FHT_ERR_HASHTABLE_EMPTY,
  FHT_ERR_SET_FAILED,
  FHT_ERR_SET_FAILED_OOM,
  FHT_ERR_COUNT,
} fht_err_t;

typedef struct zdx_fast_hashtable_get_return_val {
  fht_err_t err; // typically 4 bytes
  unsigned int val_index : 24; // another 4 bytes with padding
  FHT_VALUE_TYPE val; // depends on sizeof(FHT_VALUE_TYPE) which is user defined but if a ptr, it'll be 8 bytes
} fht_get_ret_val_t;

typedef struct zdx_fast_hashtable_set_return_val {
  fht_err_t err; // typically 4 bytes
} fht_add_ret_val_t;


#ifdef ZDX_FAST_HASHTABLE_IMPLEMENTATION

// TODO(mudit): Remove this and don't rely on malloc()
#include <stdlib.h>
#include <string.h>


#define FHT_ASSERT_NONNULL(ptr) assertm((ptr) != NULL, "Expected: ptr to not be null, Received: NULL")
#define FHT_ASSERT_RANGE(val, min, max) assertm((val) >= (min) && (val) <= (max), "Expected: value to be between %u and %u (both inclusive), Received: %u", (min), (max), (val))


// Taken from: https://stackoverflow.com/a/2351171
static inline uint32_t hash_small_string(const char str[const static 1], const uint8_t len, const uint32_t fht_cap)
{
  // TODO(mudit): Should we assume str is non-null and len is between 1 and 16 (inclusive)?
  FHT_ASSERT_NONNULL(str);
  FHT_ASSERT_RANGE(len, 1, 16);

  uint32_t hash = 0;
  uint8_t is_large = fht_cap <= 1e6;
  uint8_t multiplier = is_large ? 31 : 37;

  for (uint8_t i = 0; i < len; i++) {
    hash = multiplier * hash + str[i];
  }

  if (is_large) {
    hash += (hash >> 5);
  }

  return hash % fht_cap;
}

#define UNUSED_KEY 0
#define USED_KEY 1

static inline uint32_t get_index(const fht_t fht[const static 1], const char user_key[const static 1], const uint8_t user_key_len, const uint8_t used_status)
{
  // TODO(mudit): Should we do these checks? Cuz the fns that call get_index already should be doing these checks
  FHT_ASSERT_NONNULL(fht);
  FHT_ASSERT_NONNULL(user_key);
  // max of 16 chars is what we enforce for key length which is also somewhat enforced
  // by the key_len field in struct fht_key_t being a 5 bit field
  FHT_ASSERT_RANGE(user_key_len, 1, 16);
  FHT_ASSERT_RANGE(used_status, UNUSED_KEY, USED_KEY);

  const uint32_t fht_cap = fht->cap;
  // TODO(mudit): mod is rarely simd-ed by the compiler. Maybe use doubles to manually
  // calculate the remainder to force the compiler to simd?
  uint32_t index = hash_small_string(user_key, user_key_len, fht_cap); // index not exceeding 2^24 - 1 is enforced by fht->cap having that check in fht_init

  const fht_key_t *keys = fht->keys;
  fht_key_t curr_key = keys[index];

  // > 0 == free as key.used = 0 will match
  // > 1 == used as key.used = 1 or more will match
  if (curr_key.used == used_status) {
    return index;
  }

  index++;

  // TODO(mudit): The loop below can run for ever if the hashtable is full.
  // Should we do that check here or leave it upto functions that add to the hashmap?
  // we use open addressing
  for (;;index++) {
    if (index >= fht_cap) {
      index = 0; // wrap around <- this is also what can cause an infinite loop if hashtable being full isn't checked before calling this fn
    }

    curr_key = keys[index];

    // break on first unused slot in fht->keys
    if (curr_key.used == used_status) {
      break;
    }
  }

  return index;
}

FHT_API const char *fht_err_str(const fht_err_t err_code)
{
  FHT_ASSERT_RANGE(err_code, 1, FHT_ERR_COUNT - 1);

  static const char *const fht_err_strs[FHT_ERR_COUNT] = {
    "FHT_ERR_NONE",
    "FHT_ERR_KEY_NOT_FOUND",
    "FHT_ERR_HASHTABLE_EMPTY",
    "FHT_ERR_SET_FAILED",
    "FHT_ERR_SET_FAILED_OOM",
  };

  return fht_err_strs[err_code];
}

// TODO(mudit): Parameterize over a memory allocator
// TODO(mudit): Rewrite this correctly to not use malloc() nor hardcode things that needn't be hardcoded
FHT_API fht_t fht_init(uint32_t count)
{
  // 16777216 is 2^24 wherein 24 bits are the max number of key/val pairs this hashtable supports
  // which is addressable from 0 to 16777215
  FHT_ASSERT_RANGE(count, 1, 16777216);

  return (fht_t){
    .cap = count,
    .keys = malloc(sizeof(fht_key_t) * count),
    .values = malloc(sizeof(fht_value_t) * count),
    /* .key_suffixes = { */
    /*   .cap = count, */
    /*   .items = malloc(sizeof(fht_key_suffix_t) * count) */
    /* }, */
  };
}

FHT_API void fht_deinit(fht_t fht[const static 1])
{
  FHT_ASSERT_NONNULL(fht);

  free(fht->keys);
  free(fht->values);
  fht->cap = 0;
  fht->count = 0;
  /* free(fht->key_suffixes.items); */
  /* fht->key_suffixes.cap = 0; */
  /* fht->key_suffixes.count = 0; */
}

FHT_API void fht_empty(fht_t fht[const static 1])
{
  FHT_ASSERT_NONNULL(fht);

  fht->count = 0;
  /* fht->key_suffixes.count = 0; */
}

// TODO(mudit): for fht->cap < 1000, don't use hash function and instead just lookup/write linearly to fht->keys and fht->values
FHT_API fht_get_ret_val_t fht_get(const fht_t fht[const static 1], const char user_key[const static 1], const uint8_t user_key_len)
{
  dbg(">> key = %s, len = %u", user_key, user_key_len);

  FHT_ASSERT_NONNULL(fht);
  FHT_ASSERT_NONNULL(user_key);
  // max of 16 chars is what we enforce for key length which is also somewhat enforced
  // by the key_len field in struct fht_key_t being a 5 bit field
  FHT_ASSERT_RANGE(user_key_len, 1, 16);

  fht_get_ret_val_t result = { .err = FHT_ERR_KEY_NOT_FOUND };
  const uint32_t fht_count = fht->count;

  if (fht_count == 0) {
    result.err = FHT_ERR_HASHTABLE_EMPTY;
    return result;
  }

  const fht_key_t *keys = fht->keys;
  const fht_value_t *values = fht->values;
  const uint32_t fht_cap = fht->cap;

  // loops duplicated in if blocks to reduce last minute decision allowing for tighter loops
  if (user_key_len < 9) {
    // Get index of first used key post application of hash function on user_key
    uint32_t lookup_index = get_index(fht, user_key, user_key_len, USED_KEY);
    uint32_t iterations = 0; // this is to prevent an infinite lookup loop below

    // TODO(mudit): Should we loop unroll manually or let the compiler do it?
    do {
      if (lookup_index >= fht_cap) {
        lookup_index = 0;
      }

      const fht_key_t curr_key = keys[lookup_index++];
      const uint8_t curr_key_len = curr_key.key_len;

      // key length 0 to 4 is directly stored in the key_prefix field
      //
      // key length 0 to 9 is directly stored in key_prefix + key_suffix fields
      // and they are next to each other in memory so we can just use the key_prefix
      // as the start pointer for the string given by key_prefix and key_suffix fields together
      const char *key_prefix_and_suffix_start_ptr = curr_key.key_prefix;


      // The memcmp works even for user_key that actually matches key_prefix + key_suffix
      // because key_prefix and key_suffix in struct fht_key_t are next to each other in memory
      // this therefore uses that knowledge to just use memcmp instead of for e.g., making
      // another if branch for user_key_len > 5 and < 9
      // TODO(mudit): Can this branch be removed somehow?
      if (curr_key_len == user_key_len &&
          *user_key == *key_prefix_and_suffix_start_ptr &&
          memcmp(user_key, key_prefix_and_suffix_start_ptr, user_key_len) == 0) {
        // max of val_index is unsigned 24 bits so we use uint32 cuz C
        const uint32_t curr_key_val_index = curr_key.val_index;

        result.err = FHT_ERR_NONE;
        result.val_index = curr_key_val_index;
        result.val = values[curr_key_val_index].val;

        if (!result.err) {
          return result;
        }
      }
    } while(iterations < fht_cap);
  } else {
    // the key suffix will be stored in fht->key_suffixes array
    // TODO(mudit): Implement
  }

  return result;
}

FHT_API fht_add_ret_val_t fht_add(fht_t fht[const static 1], const char user_key[const static 1], const uint8_t user_key_len, FHT_VALUE_TYPE val)
{
  dbg(">> key = %s, len = %u", user_key, user_key_len);

  FHT_ASSERT_NONNULL(fht);
  FHT_ASSERT_NONNULL(user_key);
  // max of 16 chars is what we enforce for key length which is also somewhat enforced
  // by the key_len field in struct fht_key_t being a 5 bit field
  FHT_ASSERT_RANGE(user_key_len, 1, 16);

  fht_add_ret_val_t result = { .err = FHT_ERR_SET_FAILED };

  const uint32_t fht_count = fht->count;
  const uint32_t fht_cap = fht->cap;

  if (fht_count >= fht_cap) {
    result.err = FHT_ERR_SET_FAILED_OOM;
    return result;
  }

  const uint32_t insert_index = get_index(fht, user_key, user_key_len, UNUSED_KEY);
  fht_key_t *const keys = fht->keys;
  fht_value_t *const values = fht->values;

  if (user_key_len < 9) {
    fht_key_t *new_key = &keys[insert_index];
    fht_value_t *new_val = &values[insert_index];
    fht->count++;

    // this is used to write both key prefix and suffix as those fields
    // in struct fht_key_t are next to each other in memory
    char *const new_key_start_ptr = new_key->key_prefix;

    new_key->used = 1;
    new_key->val_index = insert_index;
    new_key->key_len = user_key_len;
    memcpy(new_key_start_ptr, user_key, user_key_len);

    new_val->val = val;

    result.err = FHT_ERR_NONE;

    return result;
  } else {
    // the key suffix (after 1st 4 chars) will need to be put in the key_suffixes array in struct fht_t
    // TODO(mudit): Implement
  }

  return result;
}

FHT_API fht_add_ret_val_t fht_update(fht_t fht[const static 1], const char user_key[const static 1], const uint8_t user_key_len, FHT_VALUE_TYPE val)
{
  // these checks are done by the call to fht_get already
  /* FHT_ASSERT_NONNULL(fht); */
  /* FHT_ASSERT_NONNULL(user_key); */
  /* FHT_ASSERT_NONZERO(user_key_len); */
  /* // max of 16 chars is what we enforce for key length which is also somewhat enforced */
  /* // by the key_len field in struct fht_key_t being a 5 bit field */
  /* FHT_ASSERT_MAXVAL(user_key_len, 16); */
  dbg(">> key = %s, len = %u", user_key, user_key_len);

  fht_get_ret_val_t get_result = fht_get(fht, user_key, user_key_len);
  fht_add_ret_val_t result = { .err = get_result.err };

  fht_value_t *values = fht->values;

  // key already exists, let's just update its value!
  if (get_result.err == FHT_ERR_NONE) {
    values[get_result.val_index].val = val;
    return result;
  }

  return result;
}

#endif // ZDX_FAST_HASHTABLE_IMPLEMENTATION
#endif // ZDX_FAST_HASHTABLE_H_
