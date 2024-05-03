/**
 * MIT License
 *
 * Copyright (c) 2024 Mudit
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
 */

#ifndef ZDX_HASHTABLE_H_
#define ZDX_HASHTABLE_H_

#ifndef HT_VALUE_TYPE
_Static_assert(false, "HT_VALUE_TYPE must be concrete defined type");
#endif // HT_VALUE_TYPE

#ifndef HT_ARENA_TYPE
/* this exists so that we can use this hashtable with built in calloc or any other non-arena style allocator */
/* this helps to satisfy the type check during compile wherever arena->err is accessed */
typedef struct zdx_ht_fake_arena { const char *err; } zdx_ht_fake_arena;
#define HT_ARENA_TYPE zdx_ht_fake_arena
#endif // HT_ARENA_TYPE

#include <stddef.h>

typedef struct hashtable_item_t ht_item_t;

typedef struct hashtable_t {
  ht_item_t *items;
  size_t length;
  size_t capacity;
} ht_t;

typedef struct hashtable_return_t {
  HT_VALUE_TYPE value;
  const char *err;
} ht_ret_t;

#ifndef HT_API
#define HT_API static // every exported function has internal linkage by default. This lib is meant to be wrapped around in your application
#endif // HT_API

/**
 * key must always be a C string i.e., it must end with \0 or all hell will break loose
 */
HT_API ht_ret_t ht_set(HT_ARENA_TYPE *const arena, ht_t ht[const static 1], const char key_cstr[const static 1], const HT_VALUE_TYPE value);
HT_API ht_ret_t ht_get(const ht_t ht[const static 1], const char key_cstr[const static 1]);
HT_API ht_ret_t ht_remove(ht_t ht[const static 1], const char key_cstr[const static 1]);
HT_API void ht_free(ht_t ht[const static 1]);
HT_API void ht_reset(ht_t ht[const static 1]);

#endif // ZDX_HASHTABLE_H_

#ifdef ZDX_HASHTABLE_IMPLEMENTATION

#if defined(HT_CALLOC) && !defined(HT_FREE)
_Static_assert(false, "HT_FREE must be defined if HT_CALLOC is");
#endif

#if defined(HT_FREE) && !defined(HT_CALLOC)
_Static_assert(false, "HT_CALLOC must be defined if HT_FREE is");
#endif

#ifndef HT_CALLOC
#include <stdlib.h>
// the first arg override is to support passing an arena for an arena allocator based alloc fn
// same for the last arg as that's needed by an arena allocator based realloc fn
#define HT_CALLOC(arena, count, size) ((void)((arena)), calloc((count), (size)))
#endif // HT_CALLOC

#ifndef HT_FREE
// can be a noop for an arena based allocator
#define HT_FREE(ptr) free((ptr))
#endif // HT_FREE

#include <stdbool.h>
#include <string.h> // memcmp and friends

#include "./zdx_util.h"

struct hashtable_item_t {
  bool occupied;
  size_t key_length;
  const char *key;
  HT_VALUE_TYPE value;
};

#ifndef HT_ASSERT
#define HT_ASSERT assertm
#endif // HT_ASSERT

#ifndef HT_MIN_LOAD_FACTOR
#define HT_MIN_LOAD_FACTOR 0.125
#endif // HT_MIN_LOAD_FACTOR

#ifndef HT_MAX_LOAD_FACTOR
#define HT_MAX_LOAD_FACTOR 0.8
#endif // HT_MAX_LOAD_FACTOR

#ifndef HT_MIN_CAPACITY
#define HT_MIN_CAPACITY 32
#endif // HT_MIN_CAPACITY

#ifndef HT_RESIZE_FACTOR
#define HT_RESIZE_FACTOR 2
#endif // HT_RESIZE_FACTOR

#define ht_dbg(label, ht) dbg("%s length %zu \t| capacity %zu \t| items %p", \
                              (label), (ht)->length, (ht)->capacity, (void *)(ht)->items)

#define ht_ret_dbg(label, ht_ret) dbg("%s value %p \t| error %s", (label), (void *)&ht_ret.value, ht_ret.err)

static inline size_t hash_djb2(const char key[const static 1], size_t len)
{
  size_t hash = 5381;
  size_t k = 33;

  for (size_t i = 0; i < len; i++) {
    hash = hash * k ^ (size_t)key[i];
    /* hash += len << 10; */
    /* hash ^= (hash * 33) >> 6; */
  }

  return hash;
}

static inline size_t hash_fnv1(const char key[const static 1], size_t len)
{
  size_t hash = 14695981039346656037UL;

  for (size_t i = 0; i < len; i++) {
    hash = (hash * 1099511628211UL) ^ (size_t)key[i];
  }

  return hash;
}

#define ht_has_collision(item, key, len)                        \
  (item)->occupied && ((item)->key_length != len ||             \
                       *(item)->key != *key ||                  \
                       memcmp((item)->key, (key), (len)) != 0)

static size_t ht_get_index(const ht_t ht[const static 1], const char key[const static 1], const size_t len)
{
  size_t idx = hash_djb2(key, len) % ht->capacity;

  if (ht_has_collision(&ht->items[idx], key, len)) {
    idx = hash_fnv1(key, len) % ht->capacity;
  }

  size_t k = 1;
  size_t max_k = 128;

  while(ht_has_collision(&ht->items[idx], key, len)) {
    idx += (k * k);
    idx = idx % ht->capacity;
    k = (k + 2) % max_k; // k + 2 instead of k << 2 as for some keys, k << 2 would result in an infinite loop here
  }

  return idx;
}

static ht_ret_t ht_resize(HT_ARENA_TYPE *arena, ht_t ht[static 1])
{
  ht_ret_t result = {0};
  float load_factor = ht->capacity ? ht->length / (float)ht->capacity : 0;

  if (load_factor > HT_MIN_LOAD_FACTOR && load_factor < HT_MAX_LOAD_FACTOR) {
    return result;
  }

  ht_dbg(">>", ht); // only print trace if something ht_resize() actually will do something

  /*
   * if items aren't initialized, allocate base size and return aka assume
   * hashtable wasn't initialized at all and just init it + return without
   * trying to copy any data (keys + values) as we assume there is none
   */
  if (ht->items == NULL || ht->capacity <= 0) {
    ht->length = 0;
    ht->capacity = 0;
    ht->items = NULL;
    ht->items = HT_CALLOC(arena, HT_MIN_CAPACITY, sizeof(*ht->items));

    if (!ht->items) {
      // TODO: Is this safe or should arena->err be duplicated? 🤔 It should be safe since arena->err are all string literals IIRC but do confirm it
      result.err = arena && arena->err ? arena->err : "Memory allocation failed";

      ht_dbg("..", ht);
      ht_ret_dbg("<<", result);
      return result;
    }

    ht->capacity = HT_MIN_CAPACITY;

    ht_dbg("..", ht);
    ht_ret_dbg("<<", result);
    return result;
  }

  dbg(".. load factor %0.4f (min: %0.4f max: %0.4f)", load_factor, HT_MIN_LOAD_FACTOR, HT_MAX_LOAD_FACTOR);

  ht_item_t *old_items = ht->items;
  size_t old_length = ht->length;

  /* grow */
  if (load_factor >= HT_MAX_LOAD_FACTOR) {
    size_t new_cap = ht->capacity * HT_RESIZE_FACTOR;

    ht->items = HT_CALLOC(arena, new_cap, sizeof(*ht->items));

    if (!ht->items) {
      // TODO: Is this safe or should arena->err be duplicated? 🤔 It should be safe since arena->err are all string literals IIRC
      result.err = arena && arena->err ? arena->err : "Memory allocation failed";

      ht_dbg("..", ht);
      ht_ret_dbg("<<", result);
      return result;
    }

    ht->capacity = new_cap;
  }

  /* shrink */
  if (load_factor <= HT_MIN_LOAD_FACTOR) {
    // TODO: maybe implemented auto shrinking
    // Remember to not shrink below HT_MIN_CAPACITY
  }

  if (ht->items != old_items) {
    size_t moved = 0;

    for (size_t i = 0; i < ht->length; i++) {
      ht_item_t *item = &old_items[i];

      if (!item->occupied) {
        continue;
      }

      size_t idx = ht_get_index(ht, item->key, item->key_length);
      ht->items[idx].occupied = item->occupied;
      ht->items[idx].key_length = item->key_length;
      ht->items[idx].key = item->key;
      ht->items[idx].value = item->value;
      moved++;
    }

    HT_ASSERT(moved == old_length, "Expected to move %zu elements but instead moved %zu", old_length, moved);

    ht->length = moved;

    HT_FREE(old_items);
  }

  ht_dbg("..", ht);
  ht_ret_dbg("<<", result);
  return result;
}

HT_API ht_ret_t ht_set(HT_ARENA_TYPE *const arena, ht_t ht[const static 1], const char key[const static 1], const HT_VALUE_TYPE value)
{
  ht_dbg(">>", ht);
  ht_ret_t result = {0};

  result = ht_resize(arena, ht);

  if (result.err) {
    ht_dbg("..", ht);
    ht_ret_dbg("<<", result);
    return result;
  }

  size_t key_length = strlen(key);
  size_t idx = ht_get_index(ht, key, key_length);
  ht_item_t *item = &ht->items[idx];

  /* inserting a new item so let's bump length of hashtable */
  if (!item->occupied) {
    ht->length++;
  }

  item->occupied = true;
  item->key_length = key_length;
  item->key = key;
  item->value = value;

  result.value = value;
  result.err = NULL;

  ht_dbg("..", ht);
  ht_ret_dbg("<<", result);
  return result;
}

HT_API ht_ret_t ht_get(const ht_t ht[const static 1], const char key[const static 1])
{
  ht_dbg(">>", ht);
  ht_ret_t result = {0};

  if (!ht->items) {
    result.err = "Key not found (empty hashtable)";

    ht_ret_dbg("<<", result);
    return result;
  }

  size_t key_length = strlen(key);

  size_t idx = ht_get_index(ht, key, key_length);

  if (!ht->items[idx].occupied) {
    result.err = "Key not found";

    ht_ret_dbg("<<", result);
    return result;
  }

  result.value = ht->items[idx].value;
  result.err = NULL;

  ht_ret_dbg("<<", result);
  return result;
}

HT_API ht_ret_t ht_remove(ht_t ht[const static 1], const char key[const static 1])
{
  ht_dbg(">>", ht);
  ht_ret_t result = {0};

  if (!ht->items) {
    result.err = "Cannot remove from an empty hashtable";

    ht_ret_dbg("<<", result);
    return result;
  }

  size_t key_length = strlen(key);

  size_t idx = ht_get_index(ht, key, key_length);

  ht->items[idx].occupied = false;
  ht->length--;

  result.err = NULL;
  result.value = ht->items[idx].value;

  ht_dbg("..", ht);
  ht_ret_dbg("<<", result);
  return result;
}

HT_API void ht_free(ht_t ht[const static 1])
{
  HT_FREE(ht->items);
  ht_reset(ht);
}

HT_API void ht_reset(ht_t ht[const static 1])
{
  ht->length = 0;
  ht->capacity = 0;
}

#endif // ZDX_HASHTABLE_IMPLEMENTATION
