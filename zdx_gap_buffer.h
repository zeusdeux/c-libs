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
#ifndef ZDX_GAP_BUFFER_
#define ZDX_GAP_BUFFER_

#pragma GCC diagnostic error "-Wnonnull"
#pragma GCC diagnostic error "-Wnull-dereference"

#include <stdlib.h>
#include <stdint.h>
#include "./zdx_util.h"

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
#define GB_INIT_LENGTH 1024 /* sizeof(char) * 1024 */
#endif // GB_INIT_LENGTH

#ifndef GB_MIN_GAP_SIZE
#define GB_MIN_GAP_SIZE 16 /* sizeof(char) * 16 */
#endif // GB_MIN_GAP_SIZE

typedef struct gap_buffer {
  char *buf;
  size_t gap_start_;
  size_t gap_end_;
  size_t length;
} gb_t;

void gb_init(gb_t gb[const static 1]);
void gb_deinit(gb_t gb[const static 1]);
void gb_move_cursor(gb_t gb[const static 1], const int64_t pos); /* pos +ve -> move right, pos -ve -> move left */
void gb_insert_char(gb_t gb[const static 1], const char c);
void gb_insert_cstr(gb_t gb[const static 1], const char cstr[static 1]);
void gb_insert_buf(gb_t gb[const static 1], void *buf, size_t length);
void gb_delete_chars(gb_t gb[const static 1], const int64_t count); /* count +ve -> delete, count -ve -> backspace */
size_t gb_get_cursor(gb_t gb[const static 1]);
char *gb_buf_as_cstr(const gb_t gb[const static 1]);
char *gb_copy_chars_as_cstr(gb_t gb[const static 1], int64_t count); /* count +ve -> copy from cursor right, count -ve -> copy from cursor left*/

#endif // ZDX_GAP_BUFFER_

#ifdef ZDX_GAP_BUFFER_IMPLEMENTATION

#include <string.h>

/* Guarded dbg trace code as it allocates */
#ifdef ZDX_TRACE_ENABLE
#define gb_dbg(label, gb)                                               \
  do {                                                                  \
    const char *buf_as_cstr = gb_buf_as_dbg_cstr(gb);                   \
    dbg("%s buf %s \t| length %zu \t| gap start %zu \t| gap end %zu",   \
        label, buf_as_cstr, gb->length, gb->gap_start_, gb->gap_end_);  \
    GB_FREE((void *)buf_as_cstr);                                       \
  } while(0)
#else
#define gb_dbg(label, gb) {}
#endif // ZDX_TRACE_ENABLE

#define gb_assert_validity(gb)                                                                                                                   \
  do {                                                                                                                                           \
    GB_ASSERT((gb) != NULL, "[zdx str] Expected: valid gap buffer instance, Received: %p", ((void *)(gb)));                                      \
    GB_ASSERT(((int64_t)gb_gap_len(gb)) >= 0, "[zdx str] Expected: gap length to be 0 or greater, Received: %lld", ((int64_t) gb_gap_len(gb)));  \
    GB_ASSERT(((int64_t)(gb)->length) >=0, "[zdx str] Expected: length is non-negative, Received: %lld", ((int64_t)(gb)->length));               \
  } while(0)

#define gb_gap_len(gb) ((gb)->gap_end_ - (gb)->gap_start_)
#define gb_buf_len_with_gap(gb) ((gb)->length + gb_gap_len(gb))

static char *gb_buf_as_dbg_cstr(const gb_t gb[const static 1])
{
  gb_assert_validity(gb);

  size_t buf_len_with_gap = gb_buf_len_with_gap(gb);
  char *str = GB_REALLOC(NULL, (buf_len_with_gap + 1) * sizeof(char));
  GB_ASSERT(str != NULL, "[zdx str] Allocation failed for buf as cstring");

  for (size_t i = 0; i < buf_len_with_gap; i++) {
    if (i >= gb->gap_start_ && i < gb->gap_end_) {
      str[i] = '.';
    } else {
      str[i] = gb->buf[i];
    }
  }
  str[buf_len_with_gap] = '\0';

  return str;
}

static void gb_resize_gap_(gb_t gb[const static 1], size_t new_gap_len)
{

  gb_dbg(">>", gb);

  gb_assert_validity(gb);
  GB_ASSERT(new_gap_len >= 0, "[zdx str] Expected: non-negative gap size to resize to, Received: %zu", new_gap_len);

  if (new_gap_len < GB_MIN_GAP_SIZE) {
    new_gap_len = GB_MIN_GAP_SIZE;
  }

  const size_t curr_gap_len = gb_gap_len(gb);
  const size_t buf_new_len = gb->length + curr_gap_len + new_gap_len;

  dbg(">> curr gap %zu \t| new gap %zu", curr_gap_len, new_gap_len);

  gb->buf = GB_REALLOC(gb->buf, buf_new_len * sizeof(char));
  GB_ASSERT(gb->buf != NULL, "[zdx str] Allocation failed for resizing gb->buf to expand gap");

  // { -> gap start marker
  // } -> gap end marker
  //
  // Example 1:
  // len = 7, new gap len = 4, gap start = 3, gap end = 3
  // abc{}cbde....
  // 012333456789a
  //
  // abc{}cbdecbde
  // 012333456789a
  //
  // abc{....}cbde
  // 012334567789a

  // Example 2:
  // len = 8, new gap len = 4, gap start = 3, gap end = 5
  // abc{..}12345....
  // 012334556789abcd
  //
  // abc{..}123412345
  // 012334556789abcd
  //
  // abc{......}12345
  // 012334567899abcd
  const void *src = (void *)(gb->buf + gb->gap_end_);
  void *dst = (void *)(gb->buf + gb->gap_end_ + new_gap_len);
  const size_t n = gb->length - gb->gap_start_;
  memmove(dst, src, n);
  gb->gap_end_ += new_gap_len;

  dbg("++ resized \t| size %zu \t| gap start %zu \t| gap end %zu", buf_new_len, gb->gap_start_, gb->gap_end_);

  gb_dbg("<<", gb);
  return;
}

char *gb_buf_as_cstr(const gb_t gb[const static 1])
{
  gb_dbg(">>", gb);
  gb_assert_validity(gb);

  char *str = GB_REALLOC(NULL, (gb->length + 1) * sizeof(char));
  GB_ASSERT(str != NULL, "[zdx str] Allocation failed for buf as cstring");

  size_t buf_len_with_gap = gb_buf_len_with_gap(gb);

  for (size_t i = 0, j = 0; i < buf_len_with_gap; i++) {
    if (i < gb->gap_start_ || i >= gb->gap_end_) {
      str[j++] = gb->buf[i];
    }
  }

  str[gb->length] = '\0';

  gb_dbg("<<", gb);
  return str;
}


void gb_init(gb_t gb[const static 1])
{
  gb_dbg(">>", gb);
  gb_assert_validity(gb);

  const size_t init_size = GB_INIT_LENGTH > GB_MIN_GAP_SIZE ? GB_INIT_LENGTH : GB_MIN_GAP_SIZE;

  /* can be replaced with gb_resize_gap_(gb, init_size) but that does checks we don't need here */
  gb->buf = GB_REALLOC(NULL, init_size * sizeof(char));
  GB_ASSERT(gb->buf != NULL, "[zdx str] Allocation failed for initializing gb->buf to default capacity");

  gb->length = 0;
  gb->gap_start_ = 0;
  gb->gap_end_ = init_size; /* gap end is beyond last valid index for gb->buf as the last valid index is a part of the gap */

  gb_dbg("<<", gb);
  return;
}

void gb_deinit(gb_t gb[const static 1])
{
  gb_dbg(">>", gb);

  GB_FREE(gb->buf);
  gb->buf = NULL;
  gb->length = 0;
  gb->gap_start_ = 0;
  gb->gap_end_ = 0;

  gb_dbg("<<", gb);
  return;
}

void gb_move_cursor(gb_t gb[const static 1], const int64_t pos)
{
  gb_dbg(">>", gb);
  gb_assert_validity(gb);

  int64_t signed_new_gap_start = gb->gap_start_ + pos;
  /* Lower bound of new gap start position aka cursor is 0 */
  signed_new_gap_start = signed_new_gap_start < 0 ? 0 : signed_new_gap_start;
  /* Upper bound of new gap start position aka cursor is one beyond end of gb->buf */
  signed_new_gap_start = (size_t) signed_new_gap_start > gb->length ? gb->length : signed_new_gap_start;

  /*
   * safely treat as size_t due to bounded assignments above
   * size_t is always atleast as wide as uint64_t which is
   * why this works. It's done to get rid of compiler warnings
   * regarding int64_t and size_t mismatch
   */
  const size_t new_gap_start = (size_t) signed_new_gap_start;
  const size_t curr_gap_len = gb_gap_len(gb);

  dbg(">> pos %lld \t| gap len %zu \t| new gap start %zu", pos, curr_gap_len, new_gap_start);

  /* noop as new cursor position is same as current */
  if (new_gap_start == gb->gap_start_) {
    gb_dbg("<<", gb);
    return;
  }

  /*
   * gap is empty so we can just update the start and end
   * without needing to do any memmoves
   */
  if (!curr_gap_len) {
    gb->gap_start_ = new_gap_start;
    gb->gap_end_ = gb->gap_start_;
    gb_dbg("<<", gb);
    return;
  }

  /* Move left */
  if (new_gap_start < gb->gap_start_) {
    dbg("!! move left")
    const void *src = (void *)(gb->buf + new_gap_start);
    void *dst = (void *)(gb->buf + new_gap_start + curr_gap_len);
    memmove(dst, src, gb->gap_start_ - new_gap_start);

    gb->gap_start_ = new_gap_start;
    gb->gap_end_ = new_gap_start + curr_gap_len;
  }

  /* Move right */
  if (new_gap_start > gb->gap_start_) {
    dbg("!! move right")
    const void *src = (void *)(gb->buf + gb->gap_end_);
    void *dst = (void *)(gb->buf + gb->gap_start_);
    memmove(dst, src, new_gap_start - gb->gap_start_);

    gb->gap_start_ = new_gap_start;
    gb->gap_end_ = new_gap_start + curr_gap_len;
  }

  gb_dbg("<<", gb);
  return;
}

void gb_insert_char(gb_t gb[const static 1], const char c)
{
  gb_dbg(">>", gb);
  dbg(">> char %c", c);

  gb_assert_validity(gb);

  if (!gb_gap_len(gb)) {
    gb_resize_gap_(gb, GB_MIN_GAP_SIZE);
  }
  gb->buf[gb->gap_start_++] = c;
  gb->length++;

  gb_dbg("<<", gb);
  return;
}

/*
 * Non c-string input will lead to undefined behavior for this function
 */
void gb_insert_cstr(gb_t gb[const static 1], const char cstr[static 1])
{
  const size_t cstr_len = strlen(cstr);

  gb_dbg(">>", gb);
  dbg(">> cstr %s \t| len(%s) %zu", cstr, cstr, cstr_len);

  gb_assert_validity(gb);
  GB_ASSERT(cstr != NULL, "[zdx str] Expected: A c string to insert, Received: %p", (void *)cstr);
  GB_ASSERT(cstr_len >= 0, "[zdx str] Expected: insertion of a string of non-zero length, Received: %zu", cstr_len);

  if (cstr_len == 0) {
    return;
  }

  while(cstr_len > gb_gap_len(gb)) {
    size_t multiple = (cstr_len / GB_MIN_GAP_SIZE) + 1; /* +1 to over allocate to reduce allocations */
    gb_resize_gap_(gb, multiple * GB_MIN_GAP_SIZE);
  }

  const void *restrict src = (void *)cstr;
  void *restrict dst = (void *)(gb->buf + gb->gap_start_);
  memcpy(dst, src, cstr_len);

  gb->gap_start_ += cstr_len;
  gb->length += cstr_len;

  gb_dbg("<<", gb);
  return;
}

void gb_insert_buf(gb_t gb[const static 1], void *buf, size_t len)
{
  gb_dbg(">>", gb);
  dbg(">> buf %p \t| length %zu", buf, len);
  gb_assert_validity(gb);
  GB_ASSERT(buf != NULL, "[zdx str] Expected: a non-NULL buf of bytes to insert, Received: %p", buf);
  GB_ASSERT(len >= 0, "[zdx str] Expected: a buf of len >= 0, Received: %zu", len);

  if (len == 0) {
    return;
  }

  while(len > gb_gap_len(gb)) {
    size_t multiple = (len / GB_MIN_GAP_SIZE) + 1; /* +1 to over allocate to reduce allocations */
    gb_resize_gap_(gb, multiple * GB_MIN_GAP_SIZE);
  }

  void *restrict dst = (void *)(gb->buf + gb->gap_start_);
  memcpy(dst, buf, len);

  gb->gap_start_ += len;
  gb->length += len;

  gb_dbg("<<", gb);
  return;
}

/* count -ve -> backspace, count +ve -> delete */
void gb_delete_chars(gb_t gb[const static 1], const int64_t count)
{
  gb_dbg(">>", gb);
  gb_assert_validity(gb);

  /* delete */
  if (count > 0) {
    int64_t new_gap_end = gb->gap_end_ + count;
    new_gap_end = new_gap_end > ((int64_t) gb_buf_len_with_gap(gb)) ? gb_buf_len_with_gap(gb) : new_gap_end;

    dbg("-- delete count %lld \t| gap end %zu \t| new gap end %lld", count, gb->gap_end_, new_gap_end);
    gb->length = gb->length - (new_gap_end - gb->gap_end_);
    gb->gap_end_ = new_gap_end;
  }

  /* backspace */
  if (count < 0) {
    int64_t new_gap_start = gb->gap_start_ + count; /* count will be -ve hence "+" */
    new_gap_start = new_gap_start < 0 ? 0 : new_gap_start;

    dbg("-- backspc count %lld \t| gap start %zu \t| new gap start %lld", count ,gb->gap_start_, new_gap_start);
    gb->length = gb->length - (gb->gap_start_ - new_gap_start);
    gb->gap_start_ = new_gap_start;
  }

  gb_dbg("<<", gb);
  return;
}

size_t gb_get_cursor(gb_t gb[const static 1])
{
  gb_assert_validity(gb);

  return gb->gap_start_;
}

/* count +ve -> copy count chars from cursor to [cursor + count] */
/* count -ve -> copy count chars from [cursor - count] to cursor */
char *gb_copy_chars_as_cstr(gb_t gb[const static 1], const int64_t count)
{
  gb_dbg(">>", gb);
  dbg(">> count %lld", count);
  gb_assert_validity(gb);

  char *empty = NULL;

  if (count != 0) {
    const size_t curr_cursor = gb_get_cursor(gb);

    if (count < 0) {
      /* trying to copy count chars backwards when cursor is at beginning of buf */
      if (curr_cursor <= 0) {
        gb_dbg("<<", gb);
        dbg("<< returning %s", empty);

        return empty;
      }

      size_t bounded_count = ((int64_t)curr_cursor + count) < 0 ? curr_cursor : llabs(count);
      dbg("!! copy left \t| chars copy count %zu", bounded_count);

      char *str = GB_REALLOC(NULL, (bounded_count + 1) * sizeof(char));
      const void *src = gb->buf + (curr_cursor - bounded_count);
      memcpy(str, src, bounded_count);

      str[bounded_count] = '\0';

      gb_dbg("<<", gb);
      dbg("<< returning %s", str);
      return str;
    } else {
      /* trying to copy count chars forwards when cursor is at end of buf */
      if (curr_cursor >= gb->length) {
        gb_dbg("<<", gb);
        dbg("<< returning %s", empty);

        return empty;
      }

      size_t bounded_count = ((size_t)count) > (gb->length - curr_cursor) ? (gb->length - curr_cursor) : llabs(count);
      dbg("!! copy right \t| chars copy count %zu", bounded_count);

      char *str = GB_REALLOC(NULL, (bounded_count + 1) * sizeof(char));
      const void *src = gb->buf + gb->gap_end_;
      memcpy(str, src, bounded_count);

      str[bounded_count] = '\0';

      gb_dbg("<<", gb);
      dbg("<< returning %s", str);
      return str;
    }
  }

  gb_dbg("<<", gb);
  dbg("<< returning %s", empty);
  return empty;
}

#endif // ZDX_GAP_BUFFER_IMPLEMENTATION
