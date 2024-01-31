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
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "./zdx_util.h"

#pragma GCC diagnostic error "-Wnonnull"

// ---- STRING BUILDER ----

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

// Calling sb_concat with a non-c string array will lead to undefined behaviour
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
// same as sb_free(sb_t *const sb) but the [static 1] enforces a pointer non-null check during compile
void sb_deinit(sb_t sb[const static 1]);

// ---- GAP BUFFER ----

#ifndef GB_REALLOC
#define GB_REALLOC realloc
#endif // GB_REALLOC

#ifndef GB_FREE
#define GB_FREE free
#endif // GB_FREE

#ifndef GB_ASSERT
#define GB_ASSERT assertm
#endif // GB_ASSERT

#ifndef GB_INIT_LENGTH
#define GB_INIT_LENGTH 1024 // sizeof(char) * 1024
#endif // GB_INIT_LENGTH

#ifndef GB_MIN_GAP_SIZE
#define GB_MIN_GAP_SIZE 16 // sizeof(char) * 16
#endif // GB_MIN_GAP_SIZE

typedef struct gap_buffer {
  char *buf;
  size_t gap_start_;
  size_t gap_end_;
  size_t length;
} gb_t;

void gb_init(gb_t gb[const static 1]);
void gb_deinit(gb_t gb[const static 1]);
void gb_move_cursor(gb_t gb[const static 1], const int64_t pos);
void gb_insert_char(gb_t gb[const static 1], const char c);
void gb_insert_cstr(gb_t gb[const static 1], const char cstr[static 1], const size_t cstr_len);
void gb_delete_chars(gb_t gb[const static 1], const uint64_t count); // count +ve -> backspace, count -ve -> delete
char *gb_buf_as_cstr(const gb_t gb[const static 1]);

#endif // ZDX_STR_H_

#ifdef ZDX_STR_IMPLEMENTATION

// ---- STRING BUILDER IMPLEMENTATION ----

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

    dbg("<< (%s, %zu)", cstr, cstr_len);

    if (reqd_capacity > sb->capacity) {
      sb_resize_(sb, reqd_capacity);
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

size_t sb_append_buf(sb_t sb[const static 1], const char *buf, const size_t buf_size)
{
  SB_ASSERT(sb != NULL, "[zdx_str] Expected: valid string builder instance, Received: %p", (void *)sb);
  SB_ASSERT(buf != NULL, "[zdx_str] Expected: valid buffer, Received: %p", (void *)buf);
  SB_ASSERT(buf_size > 0, "[zdx_str] Expected: buf size great than 0, Received: %zu", buf_size);

  dbg("<< (%p, %zu)", (void *)buf, buf_size);

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

  dbg(">> (%s, %zu)", sb->str, sb->length);

  return sb->length;
}

void sb_deinit(sb_t sb[const static 1])
{
  SB_FREE((void *)sb->str);
  sb->str = NULL;
  sb->length = 0;
  sb->capacity = 0;
}


// ---- GAP BUFFER IMPLEMENTATION ----

#define gb_dbg(label, gb)                                               \
  do {                                                                  \
    const char *buf_as_cstr = gb_buf_as_dbg_cstr(gb);                       \
    dbg("%s buf %s \t| length %zu \t| gap start %zu \t| gap end %zu",   \
        label, buf_as_cstr, gb->length, gb->gap_start_, gb->gap_end_);  \
    GB_FREE((void *)buf_as_cstr);                                       \
  } while(0)

#define gb_len_with_gap(gb) (((gb)->length + ((gb)->gap_end_ - (gb)->gap_start_)) ? ((gb)->length + ((gb)->gap_end_ - (gb)->gap_start_) + 1) : 0)

static const char *gb_buf_as_dbg_cstr(const gb_t gb[const static 1])
{
  GB_ASSERT(gb != NULL, "[zdx str] Expected: valid gap buffer instance, Received: %p", (void *)gb);

  // abc{....}cbde
  // 012334566789a
  // len = 7, start = 3, end = 6
  // buf_len_with_gap = 7 + (6 - 3) + 1
  size_t buf_len_with_gap = gb_len_with_gap(gb);

  if (buf_len_with_gap) {
    buf_len_with_gap += 1;
  }

  char *str = GB_REALLOC(NULL, (buf_len_with_gap + 1) * sizeof(char));
  GB_ASSERT(str != NULL, "[zdx str] Allocation failed for buf as cstring");

  for (size_t i = 0; i < buf_len_with_gap; i++) {
    if (i >= gb->gap_start_ && i <= gb->gap_end_) {
      str[i] = '.';
    } else {
      str[i] = gb->buf[i];
    }
  }

  str[buf_len_with_gap] = '\0';

  return str;
}


static void gb_resize_gap_(gb_t gb[const static 1], size_t gap_size)
{
  gb_dbg(">>", gb);
  dbg(">> gap size %zu", gap_size);

  GB_ASSERT(gb != NULL, "[zdx str] Expected: valid gap buffer instance, Received: %p", (void *)gb);
  GB_ASSERT(gap_size >= 0, "[zdx str] Expected: non-negative gap size, Received: %zu", gap_size);

  if (gap_size < GB_MIN_GAP_SIZE) {
    gap_size = GB_MIN_GAP_SIZE;
  }

  // abc{}cbde
  // resize gb->buf to include GB_MIN_GAP_SIZE
  const size_t new_size = gb->length + gap_size;
  gb->buf = GB_REALLOC(gb->buf, new_size);
  GB_ASSERT(gb->buf != NULL, "[zdx str] Allocation failed for resizing gb->buf to expand gap");

  // Example 1:
  // abc{}cbde....
  // 0123345678
  // len = 7
  // dst = buf + gap_start
  // src = buf + (gap_end + (len - gap_start) + 1)
  // abc{....}cbde
  // 012334566789a
  //
  // Example 2:
  // abc{..}12345....
  // 012334456789a
  // len = 8
  // dst = buf + gap_start
  // src = buf + (gap_end + (len - gap_start) + 1)
  // abc{......}12345
  // 0123344567789abc
  void *dst = (void *)(gb->buf + gb->gap_start_);
  const void *src = (void *)(gb->buf + (gb->gap_end_ + (gb->length - gb->gap_start_) + 1));
  memmove(dst, src, GB_MIN_GAP_SIZE);
  gb->gap_end_ += gap_size;

  dbg("++ resized \t| size %zu \t| gap start %zu \t| gap end %zu", new_size, gb->gap_start_, gb->gap_end_);

  gb_dbg("<<", gb);

  // done
  return;
}

char *gb_buf_as_cstr(const gb_t gb[const static 1])
{
  gb_dbg(">>", gb);
  GB_ASSERT(gb != NULL, "[zdx str] Expected: valid gap buffer instance, Received: %p", (void *)gb);

  char *str = GB_REALLOC(NULL, gb->length * sizeof(char));
  GB_ASSERT(str != NULL, "[zdx str] Allocation failed for buf as cstring");

  size_t buf_len_with_gap = gb_len_with_gap(gb);
  if (buf_len_with_gap) {
    buf_len_with_gap += 1;
  }

  for (size_t i = 0; i < buf_len_with_gap; i++) {
    if (i < gb->gap_start_ || i > gb->gap_end_) {
      str[i] = gb->buf[i];
    }
  }

  str[gb->length] = '\0';

  gb_dbg("<<", gb);
  return str;
}


void gb_init(gb_t gb[const static 1])
{
  gb_dbg(">>", gb);
  GB_ASSERT(gb != NULL, "[zdx str] Expected: valid gap buffer instance, Received: %p", (void *)gb);

  const size_t init_size = GB_INIT_LENGTH > GB_MIN_GAP_SIZE ? GB_INIT_LENGTH : GB_MIN_GAP_SIZE;

  gb->gap_start_ = 0;
  gb->gap_end_ = init_size - 1;
  gb->length = 0;
  gb->buf = GB_REALLOC(NULL, init_size * sizeof(char));
  GB_ASSERT(gb->buf != NULL, "[zdx str] Allocation failed for initializing gb->buf to default capacity");

  gb_dbg("<<", gb);
  return;
}

void gb_deinit(gb_t gb[const static 1])
{
  gb_dbg(">>", gb);

  GB_FREE(gb->buf);
  gb->gap_start_ = 0;
  gb->gap_end_ = 0;
  gb->length = 0;
  gb->buf = NULL;

  gb_dbg("<<", gb);
  return;
}

void gb_move_cursor(gb_t gb[const static 1], const int64_t pos)
{
  (void) gb;
  (void) pos;
}

void gb_insert_char(gb_t gb[const static 1], const char c)
{
  gb_dbg(">>", gb);
  dbg(">> char %c", c);

  GB_ASSERT(gb != NULL, "[zdx str] Expected: valid gap buffer instance, Received: %p", (void *)gb);

  if (gb->gap_start_ > gb->gap_end_) {
    gb_resize_gap_(gb, GB_MIN_GAP_SIZE);
  }
  gb->buf[gb->gap_start_++] = c;
  gb->length++;

  gb_dbg("<<", gb);
  return;
}

void gb_insert_cstr(gb_t gb[const static 1], const char cstr[static 1], const size_t cstr_len);
void gb_delete_chars(gb_t gb[const static 1], const uint64_t count); // count +ve -> backspace, count -ve -> delete

#endif // ZDX_STR_IMPLEMENTATION
