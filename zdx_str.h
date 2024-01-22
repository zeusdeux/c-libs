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

#ifndef ZDX_STR_H_
#define ZDX_STR_H_

#pragma GCC diagnostic error "-Wnonnull"
#pragma GCC diagnostic error "-Wnull-dereference"
#pragma GCC diagnostic ignored "-Wgnu-statement-expression-from-macro-expansion"
#pragma GCC diagnostic ignored "-Wmacro-redefined"

/**
 * This lib should contain string builder functionality (with interfaces for buf as well as cstr)
 * and string view functionality. It should also contain convenience methods for reading and
 * writing full files, read and write to files or buffers by line, etc. Maybe even replacements
 * for splitting a string by token (as strtok is poo) and lazily getting parts back, etc.
 */
/**
 * ALWAYS compile with -Werror=nonnull -Werror-null-dereference when using this library!!
 * Without that, some NULL checks just get optimized away in release builds.
 */
#include <stdlib.h>
#include <string.h>
#include "./zdx_util.h"

#ifndef SB_RESIZE_FACTOR
#define SB_RESIZE_FACTOR 2
#endif

#ifndef SB_MIN_CAPACITY
#define SB_MIN_CAPACITY 16
#endif

typedef struct {
  size_t capacity;
  size_t length;
  char *str;
} sb_t;

// Calling sb_concat with a non-c string array will lead to undefined behaviour
#define sb_concat(sb, arr)                      \
  sb_append_cstrs_((sb),                        \
                   zdx_arr_len((arr)),          \
                   (arr))
#define sb_append(sb, ...)                                                     \
  sb_append_cstrs_((sb),                                                       \
                   zdx_arr_len(((__typeof__((__VA_ARGS__))[]){__VA_ARGS__})),  \
                   zdx_first_arg(__VA_ARGS__, "<FILLER>") ? ((const char **)(__typeof__((__VA_ARGS__))[]){__VA_ARGS__}) : NULL)
//                                            ^ this <FILLER> is for when __VA_ARGS__ expands to only one argument and zdx_first_arg

//                    sb_t const* sb         , const size_t count, const char **cstrs  // less strict analogs in pointer language
size_t sb_append_cstrs_(sb_t sb[const static 1], const size_t cstrs_count, const char *cstrs[static cstrs_count]);
// same as sb_free(sb_t *const sb) but the [static 1] enforces a pointer non-null check during compile
void sb_free(sb_t sb[const static 1]);

#endif // ZDX_STR_H_

#ifdef ZDX_STR_IMPLEMENTATION

// Using sb_append_cstrs_ with non-c strings will lead to undefined behavior
// Returns new length of cstring it has stored inside
size_t sb_append_cstrs_(sb_t sb[const static 1], const size_t cstrs_count, const char *cstrs[static cstrs_count])
{
  assertm(sb != NULL, "[zdx_str] Expected: valid string builder instance, Received: %p", (void *)sb);
  assertm(cstrs_count > 0, "[zdx_str] Expected: number of cstrings to insert to be > 0, Received: %zu", cstrs_count);
  assertm(cstrs != NULL, "[zdx_str] Expected: array of cstring to insert to be non-NULL, Received: %p", (void *)cstrs);

  for (size_t i = 0; i < cstrs_count; i++) {
    const char *cstr = cstrs[i];
    size_t cstr_len = strlen(cstr);

    dbg("<< (%s, %zu)", cstr, cstr_len);

    if ((sb->length + cstr_len) > sb->capacity) {
      if (sb->capacity == 0) {
        sb->capacity = SB_MIN_CAPACITY;
      }
      while(sb->capacity < (cstr_len + sb->length)) {
        sb->capacity *= SB_RESIZE_FACTOR;
      }
      sb->str = (char *)realloc((void *)sb->str, sb->capacity * (sizeof(char)) + 1);
      dbg("resized (capacity %zu)", sb->capacity);
    }

    size_t start = sb->length ? sb->length : 0;

    for (size_t j = 0; j < cstr_len; j++) {
      sb->str[start + j] = cstr[j];
    }

    sb->str[start + cstr_len] = '\0';
    sb->length += cstr_len;

    dbg(">> (%s, %zu)", sb->str, sb->length);
  }

  return sb->length;
}

void sb_free(sb_t sb[const static 1])
{
  free((void *)sb->str);
  sb->str = NULL;
  sb->length = 0;
  sb->capacity = 0;
}

#endif // ZDX_STR_IMPLEMENTATION
