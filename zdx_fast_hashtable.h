/**
 * MIT License
 *
 * Copyright (c) 2024 Mudit Ameta
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *
 * DESCRIPTION
 *
 * A simple hashtable with max key length of 13 ASCII chars that must have a
 * pre-defined max key/value pair count when initialized.
 *
 * CONSTRAINTS
 *
 * 1. Max key length allowed is 13 ASCII characters (13 bytes) and can be configured
 *    at include time using FHT_MAX_KEYLEN macro. (Recommended range: 8 to 13)
 * 2. Max no., of keys allowed is 1_048_576 (2^20) as beyond that perf of this strategy
 *    becomes unacceptable to me
 * 3. Max count of key/value pairs that will be stored need to be given when the
 *    hashtable is initialized
 */
#ifndef ZDX_FAST_HASHTABLE_H_
#define ZDX_FAST_HASHTABLE_H_

#include <stdint.h>


// ------------------------- MACROS -------------------------

#if !defined(FHT_VALUE_TYPE)
_Static_assert(0, "FHT_VALUE_TYPE must be defined to use zdx_fast_hashtable.h");
#endif

#ifndef FHT_API
#define FHT_API
#endif

#ifndef FHT_ASSERT
#include "zdx_util.h"
#define FHT_ASSERT assertm
#endif

// max bits for key count imply max keycount
#define FHT_MAX_KEYCOUNT_BITS 20
// max key count implies max val count
#define FHT_MAX_KEYCOUNT 1 << FHT_MAX_KEYCOUNT_BITS
// max key len as we only have 4 bits for key len balanced with perf (hence 13 and not 15)
#ifndef FHT_MAX_KEYLEN
#define FHT_MAX_KEYLEN 13
#endif // FHT_MAX_KEYLEN
_Static_assert(FHT_MAX_KEYLEN > 0 && FHT_MAX_KEYLEN < 14, "FHT_MAX_KEYLEN should be between 1 and 13");


// -------------------- TYPE DECLARATIONS --------------------

typedef struct zdx_fast_hashtable_key {
  // 4 bits represent the length of the key
  //   Max we allow is 13 ASCII chars
  //   Also, key_len == 0 means key is unused
  unsigned int key_len : 4;

  // 20 bits represent the index of value in the values array
  //   Max count of key/value pairs this hashtable can store therefore is 2^20
  //   i.e., 1048576 or 0 to 1046575 in indices
  unsigned int val_index : FHT_MAX_KEYCOUNT_BITS;

  // FHT_MAX_KEYLEN bytes (1 to 13) to hold ASCII characters
  char key[FHT_MAX_KEYLEN];
} fht_key_t;

typedef struct zdx_fast_hashtable_value {
  FHT_VALUE_TYPE val;
} fht_value_t;

typedef struct zdx_fast_hashtable {
    uint32_t cap;
    uint32_t count;
    fht_key_t *keys;
    fht_value_t *values;
} fht_t;

typedef enum {
  FHT_KEY_STATUS_UNUSED = 0,
  FHT_KEY_STATUS_USED = 1,
  FHT_KEY_STATUS_COUNT,
} fht_key_status_t;

typedef enum zdx_fast_hashtable_error {
  FHT_ERR_NONE = 0,
  FHT_ERR_KEY_NOT_FOUND,
  FHT_ERR_HASHTABLE_EMPTY,
  FHT_ERR_ADD_FAILED,
  FHT_ERR_ADD_FAILED_OOM,
  FHT_ERR_COUNT,
} fht_err_t;

typedef struct zdx_fast_hashtable_get_return_val {
  fht_err_t err; // typically 4 bytes
  FHT_VALUE_TYPE val; // depends on sizeof(FHT_VALUE_TYPE) which is user defined but if a ptr, it'll be 8 bytes
} fht_get_ret_val_t;

typedef struct zdx_fast_hashtable_add_return_val {
  fht_err_t err; // typically 4 bytes
} fht_add_ret_val_t;


// -------------------- FUNCTION DECLARATIONS --------------------

FHT_API const char *fht_err_str(const fht_err_t err_code);

FHT_API fht_t fht_init(uint32_t count);
FHT_API void fht_deinit(fht_t fht[const static 1]);
FHT_API void fht_empty(fht_t fht[const static 1]);

FHT_API fht_get_ret_val_t fht_get(const fht_t fht[const static 1], const char user_key[const static 1], const uint8_t user_key_len);
FHT_API fht_add_ret_val_t fht_add(fht_t fht[const static 1], const char user_key[const static 1], const uint8_t user_key_len, FHT_VALUE_TYPE val);
FHT_API fht_add_ret_val_t fht_update(fht_t fht[const static 1], const char user_key[const static 1], const uint8_t user_key_len, FHT_VALUE_TYPE val);


// ------------------ FUNCTION IMPLEMENTATIONS -------------------

#ifdef ZDX_FAST_HASHTABLE_IMPLEMENTATION


// TODO(mudit): Remove this and don't rely on malloc()
#include <stdlib.h>
#include <string.h>


#define FHT_ASSERT_NONNULL(ptr) assertm((ptr) != NULL, "Expected: ptr to not be null, Received: NULL")
#define FHT_ASSERT_RANGE(val, min, max) assertm((val) >= (min) && (val) <= (max), "Expected: value to be between %u and %u (both inclusive), Received: %u", (min), (max), (val))


// -------------------- PRIVATE FUNCTIONS --------------------

// Base function taken from: https://stackoverflow.com/a/2351171
// and then modified based on benchmarking data.
static inline uint32_t fht_hash_small_string_(const char str[const static 1], const uint8_t len, const uint32_t fht_cap)
{
  // TODO(mudit): should we remove these asserts and assume they are done before calling this fn?
  FHT_ASSERT_NONNULL(str);
  FHT_ASSERT_RANGE(len, 1, FHT_MAX_KEYLEN);

  uint32_t hash = 0;
  uint8_t is_large = fht_cap < 1e6;
  uint8_t multiplier = is_large ? 31 : 37;
  uint8_t shift = is_large ? 5 : 0;

  for (uint8_t i = 0; i < len; i++) {
    hash = multiplier * hash + str[i];
  }

  hash += (hash >> shift) + len;

  // TODO(mudit): mod is rarely simd-ed by the compiler. Maybe use doubles to manually
  // calculate the remainder to force the compiler to simd?
  return hash % fht_cap;
}

static inline uint32_t get_hashed_index_(const fht_t fht[const static 1], const char user_key[const static 1], const uint8_t user_key_len, const fht_key_status_t key_status)
{
  // TODO(mudit): Should we do these checks? Cuz the fns that call get_index already should be doing these checks
  FHT_ASSERT_NONNULL(fht);
  FHT_ASSERT_NONNULL(user_key);
  FHT_ASSERT_RANGE(user_key_len, 1, FHT_MAX_KEYLEN);

  const uint32_t fht_cap = fht->cap;
  uint32_t index = fht_hash_small_string_(user_key, user_key_len, fht_cap);

  const fht_key_t *keys = fht->keys;
  fht_key_t curr_key = keys[index];
  uint8_t curr_key_status = curr_key.key_len > 0;

  if (curr_key_status == key_status) {
    return index;
  }

  index++;

  // TODO(mudit): The loop below can run for ever if the hashtable is full and
  //              you're looking for a free slot or if the hashtable is empty and
  //              you're looking for a filled slot.
  //              Currently we do checks to prevent this infinite loops in the callers
  //              of this fn. Should we do that check here or leave it in the callers?
  //
  // we use open addressing
  for (;;index++) {
    if (index >= fht_cap) {
      index = 0; // wrap around <- this is also what can cause an infinite loop if hashtable being full isn't checked before calling this fn
    }

    curr_key = keys[index];
    curr_key_status = curr_key.key_len > 0;

    if (curr_key_status == key_status) {
      break;
    }
  }

  return index;
}

typedef struct zdx_fast_hashtable_return_index {
  fht_err_t err; // typically 4 bytes
  unsigned int val_index : FHT_MAX_KEYCOUNT_BITS; // no of keys == no of values. This is another 4 bytes with padding
} fht_ret_index_t;

static fht_ret_index_t fht_get_val_index_(const fht_t fht[const static 1], const char user_key[const static 1], const uint8_t user_key_len)
{
  dbg(">> key = %s, len = %u", user_key, user_key_len);

  FHT_ASSERT_NONNULL(fht);
  FHT_ASSERT_NONNULL(user_key);
  FHT_ASSERT_RANGE(user_key_len, 1, FHT_MAX_KEYLEN);

  // result.val_index here is 0 and should always be a valid index
  // as result.err is what disambiguates a valid val_index from an
  // invalid one
  fht_ret_index_t result = { .err = FHT_ERR_KEY_NOT_FOUND };
  const uint32_t fht_count = fht->count;

  if (fht_count == 0) {
    result.err = FHT_ERR_HASHTABLE_EMPTY;
    return result;
  }

  const fht_key_t *keys = fht->keys;
  const uint32_t fht_cap = fht->cap;

  // Get index of first used key post application of hash function on user_key
  uint32_t lookup_index = get_hashed_index_(fht, user_key, user_key_len, FHT_KEY_STATUS_USED);
  uint32_t iterations = 0; // this is to prevent an infinite lookup loop below

  // TODO(mudit): Should we loop unroll manually or let the compiler do it?
  do {
    // Tested using `% fht_cap` instead of this branch and that is WAY slower
    // so taking the potential branch mispredict hit here instead
    if (lookup_index >= fht_cap) {
      lookup_index = 0;
    }

    const fht_key_t curr_key = keys[lookup_index++];
    const uint8_t curr_key_len = curr_key.key_len;
    const char *curr_key_start_ptr = curr_key.key;

    // TODO(mudit): Can this branch be removed somehow?
    if (curr_key_len == user_key_len &&
        *user_key == *curr_key_start_ptr &&
        memcmp(user_key, curr_key_start_ptr, user_key_len) == 0) {
      result.err = FHT_ERR_NONE;
      result.val_index = curr_key.val_index;

      return result;
    }
  } while(iterations < fht_cap);

  return result;
}


// -------------------- PUBLIC FUNCTIONS --------------------

FHT_API const char *fht_err_str(const fht_err_t err_code)
{
  FHT_ASSERT_RANGE(err_code, 1, FHT_ERR_COUNT - 1);

  static const char *const fht_err_strs[FHT_ERR_COUNT] = {
    "FHT_ERR_NONE",
    "FHT_ERR_KEY_NOT_FOUND",
    "FHT_ERR_HASHTABLE_EMPTY",
    "FHT_ERR_ADD_FAILED",
    "FHT_ERR_ADD_FAILED_OOM",
  };

  return fht_err_strs[err_code];
}

// TODO(mudit): Parameterize over a memory allocator
// TODO(mudit): Rewrite this correctly to not use malloc() nor hardcode things that needn't be hardcoded
FHT_API fht_t fht_init(uint32_t count)
{
  FHT_ASSERT_RANGE(count, 1, FHT_MAX_KEYCOUNT);

  return (fht_t){
    .cap = count,
    .keys = malloc(sizeof(fht_key_t) * count),
    .values = malloc(sizeof(fht_value_t) * count),
  };
}

FHT_API void fht_deinit(fht_t fht[const static 1])
{
  FHT_ASSERT_NONNULL(fht);

  free(fht->keys);
  free(fht->values);
  fht->cap = 0;
  fht->count = 0;
}

FHT_API void fht_empty(fht_t fht[const static 1])
{
  FHT_ASSERT_NONNULL(fht);

  fht->count = 0;
}

FHT_API fht_get_ret_val_t fht_get(const fht_t fht[const static 1], const char user_key[const static 1], const uint8_t user_key_len)
{
  const fht_ret_index_t get_result = fht_get_val_index_(fht, user_key, user_key_len);

  const fht_get_ret_val_t result = {
    .err = get_result.err,
    .val = fht->values[get_result.val_index].val
  };

  return result;
}

FHT_API fht_add_ret_val_t fht_add(fht_t fht[const static 1], const char user_key[const static 1], const uint8_t user_key_len, FHT_VALUE_TYPE val)
{
  dbg(">> key = %s, len = %u", user_key, user_key_len);

  FHT_ASSERT_NONNULL(fht);
  FHT_ASSERT_NONNULL(user_key);
  FHT_ASSERT_RANGE(user_key_len, 1, FHT_MAX_KEYLEN);

  fht_add_ret_val_t result = {0};

  const uint32_t fht_count = fht->count;
  const uint32_t fht_cap = fht->cap;

  if (fht_count >= fht_cap) {
    result.err = FHT_ERR_ADD_FAILED_OOM;
    return result;
  }

  const uint32_t insert_index = get_hashed_index_(fht, user_key, user_key_len, FHT_KEY_STATUS_UNUSED);
  fht_key_t *const keys = fht->keys;
  fht_value_t *const values = fht->values;

  fht_key_t *const new_key = &keys[insert_index];
  fht_value_t *const new_val = &values[insert_index];

  char *const new_key_start_ptr = new_key->key;

  // add key
  new_key->val_index = insert_index;
  new_key->key_len = user_key_len;
  memcpy(new_key_start_ptr, user_key, user_key_len);

  // add value
  new_val->val = val;

  // increment count of stored key/vals
  fht->count++;

  result.err = FHT_ERR_NONE;

  return result;
}

FHT_API fht_add_ret_val_t fht_update(fht_t fht[const static 1], const char user_key[const static 1], const uint8_t user_key_len, FHT_VALUE_TYPE val)
{
  dbg(">> key = %s, len = %u", user_key, user_key_len);

  fht_ret_index_t get_result = fht_get_val_index_(fht, user_key, user_key_len);
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
