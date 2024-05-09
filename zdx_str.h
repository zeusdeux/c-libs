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
 */
#ifndef ZDX_STR_H_
#define ZDX_STR_H_

#pragma GCC diagnostic error "-Wnonnull"
#pragma GCC diagnostic error "-Wnull-dereference"
#pragma GCC diagnostic error "-Wsign-conversion"

#include <stdlib.h>
/* needed for macros used in the header section */
#include "./zdx_util.h"

/**
 * This lib should contain string builder functionality (with interfaces for buf as well as cstr)
 * and string view functionality. It should also contain convenience methods for reading and
 * writing full files, read and write to files or buffers by line, etc. Maybe even replacements
 * for splitting a string by token (as strtok is poo) and lazily getting parts back, etc.
 */

/* ---- STRING BUILDER ---- */

#ifndef SB_REALLOC
#define SB_REALLOC realloc
#endif // SB_REALLOC

#ifndef SB_FREE
#define SB_FREE free
#endif // SB_FREE

#ifndef SB_ASSERT
#define SB_ASSERT assertm
#endif // SB_ASSERT

#ifndef SB_RESIZE_FACTOR
#define SB_RESIZE_FACTOR 2
#endif

#ifndef SB_MIN_CAPACITY
#define SB_MIN_CAPACITY 16
#endif

typedef struct string_builder {
  size_t capacity;
  size_t length;
  char *str;
} sb_t;

/* Calling sb_concat with a non-c string array will lead to undefined behaviour */
#define sb_concat(sb, arr)                              \
  sb_append_cstrs_((sb),                                \
                   (arr) ? zdx_arr_len(arr) : 0,        \
                   (arr) ? (arr) : NULL);

#define sb_append(sb, ...)                                                                               \
  _Pragma("GCC diagnostic push")                                                                         \
  _Pragma("GCC diagnostic ignored \"-Wunused-value\"")                                                   \
  sb_append_cstrs_((sb),                                                                                 \
                   (__VA_ARGS__) ? zdx_arr_len(((__typeof__((__VA_ARGS__))[]){__VA_ARGS__})) : 0,        \
                   (__VA_ARGS__) ? ((const char **)(__typeof__((__VA_ARGS__))[]){__VA_ARGS__}) : NULL);  \
  _Pragma("GCC diagnostic pop")

size_t sb_append_buf(sb_t sb[const static 1], const char *buf, const size_t buf_size);
/* same as sb_free(sb_t *const sb) but the [static 1] enforces a pointer non-null check during compile */
void sb_deinit(sb_t sb[const static 1]);

#endif // ZDX_STR_H_

// ----------------------------------------------------------------------------------------------------------------

#ifdef ZDX_STR_IMPLEMENTATION

#include <string.h>

/* ---- STRING BUILDER IMPLEMENTATION ---- */

static void sb_resize_(sb_t sb[const static 1], const size_t reqd_capacity)
{
  if (sb->capacity <= 0) {
    sb->capacity = SB_MIN_CAPACITY;
  }
  while(sb->capacity < reqd_capacity) {
    sb->capacity *= SB_RESIZE_FACTOR;
  }
  sb->str = SB_REALLOC(sb->str, sb->capacity * sizeof(char));
  SB_ASSERT(sb->str != NULL, "[zdx str] string builder resize allocation failed");

  dbg("++ resized (capacity %zu)", sb->capacity);
  return;
}

// Using sb_append_cstrs_ with non-c strings will lead to undefined behavior
// Returns new length of cstring it has stored inside
//                      sb_t const* sb         , const size_t count,       const char **cstrs  // less strict pointer analogs
size_t sb_append_cstrs_(sb_t sb[const static 1], const size_t cstrs_count, const char *cstrs[static cstrs_count])
{
  SB_ASSERT(sb != NULL, "[zdx_str] Expected: valid string builder instance, Received: %p", (void *)sb);
  SB_ASSERT(cstrs_count > 0, "[zdx_str] Expected: number of cstrings to insert to be > 0, Received: %zu", cstrs_count);
  SB_ASSERT(cstrs != NULL, "[zdx_str] Expected: array of cstring to insert to be non-NULL, Received: %p", (void *)cstrs);

  for (size_t i = 0; i < cstrs_count; i++) {
    const char *cstr = cstrs[i];
    size_t cstr_len = strlen(cstr);
    // +1 for preemptively growing sb->str in case we need to append a '\0'
    size_t reqd_capacity = sb->length + cstr_len + 1;

    dbg(">> cstr %s \t| len %zu \t| reqd cap %zu \t| sb->len %zu", cstr, cstr_len, reqd_capacity, sb->length);

    if (reqd_capacity > sb->capacity) {
      sb_resize_(sb, reqd_capacity);
    }

    size_t start = sb->length ? sb->length : 0;

    for (size_t j = 0; j < cstr_len; j++) {
      sb->str[start + j] = cstr[j];
    }

    sb->str[start + cstr_len] = '\0';
    sb->length += cstr_len;

    dbg("<< sb->str %s \t| sb->len %zu", sb->str, sb->length);
  }

  return sb->length;
}

size_t sb_append_buf(sb_t sb[const static 1], const char *buf, const size_t buf_size)
{
  SB_ASSERT(sb != NULL, "[zdx_str] Expected: valid string builder instance, Received: %p", (void *)sb);
  SB_ASSERT(buf != NULL, "[zdx_str] Expected: valid buffer, Received: %p", (void *)buf);
  SB_ASSERT(buf_size > 0, "[zdx_str] Expected: buf size great than 0, Received: %zu", buf_size);

  dbg(">> (%p, %zu)", (void *)buf, buf_size);

  // +1 for preemptively growing sb->str in case we need to append a '\0'
  const size_t reqd_capacity = sb->length + buf_size + 1;

  if (reqd_capacity > sb->capacity) {
    sb_resize_(sb, reqd_capacity);
  }

  for (size_t i = 0; i < buf_size; i++) {
    sb->str[sb->length++] = buf[i];
  }

  // to make sure sb->str is always a valid c string
  if (buf[buf_size - 1] != '\0') {
    sb->str[sb->length] = '\0';
  }

  dbg("<< (%s, %zu)", sb->str, sb->length);

  return sb->length;
}

void sb_deinit(sb_t sb[const static 1])
{
  SB_FREE((void *)sb->str);
  sb->str = NULL;
  sb->length = 0;
  sb->capacity = 0;
  return;
}

#endif // ZDX_STR_IMPLEMENTATION
