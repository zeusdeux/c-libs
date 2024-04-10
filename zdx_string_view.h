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

/**
 * THIS LIBRARY SHOULD NEVER ALLOCATE!
 * It should only provide readonly views of the underlying buffer.
 * Also, it should use only automatic storage so no pointers
 * should be passed in to any functions unless the function needs to
 * "return" multiple values by side effecting the incoming
 * string_view value - for e.g., sv_split_by_char that returns the
 * first split chunk and also updates the incoming sv_t value to point
 * after the chunk that was split out.
 * Keep it simple!
 */
#ifndef ZDX_STRING_VIEW_H_
#define ZDX_STRING_VIEW_H_

#pragma GCC diagnostic error "-Wsign-conversion"

#include <stddef.h>
#include <stdbool.h>

/**
 * opaque type as we don't want any consumers to directly initialize this
 * struct but rather only consume it from one of the functions below
 **/
typedef struct string_view sv_t;

#define SV_FMT "%.*s"
#define sv_fmt_args(sv) (int)(sv).length, (sv).buf

sv_t sv_from_buf(const char* buf, const size_t len);
sv_t sv_from_cstr(const char* str);
bool sv_begins_with_word_buf(sv_t sv, const char *buf, const size_t len);
bool sv_begins_with_word_cstr(sv_t sv, const char *str);
bool sv_eq_cstr(sv_t sv, const char *str);
bool sv_eq_sv(const sv_t sv1, const sv_t sv2);
bool sv_is_empty(const sv_t sv);
bool sv_has_char(const sv_t sv1, const char c);
sv_t sv_trim_left(sv_t sv);
sv_t sv_trim_right(sv_t sv);
sv_t sv_trim(sv_t sv);
sv_t sv_split_by_char(sv_t *const sv, char delim);
sv_t sv_split_from_idx(sv_t *const sv, const size_t from); /* including idx from */
sv_t sv_split_until_idx(sv_t *const sv, const size_t until); /* excluding idx until */

#endif // ZDX_STRING_VIEW_H_

#ifdef ZDX_STRING_VIEW_IMPLEMENTATION

#include <ctype.h>
#include <string.h>
#include "./zdx_util.h"

/* These defines should only be overriden in the file */
/* that includes the implementation */
#ifndef SV_ASSERT
#define SV_ASSERT assertm
#endif // SV_ASSERT

typedef struct string_view {
  const char *buf;
  size_t length;
} sv_t;

#define sv_dbg(label, sv) dbg("%s buf \""SV_FMT"\" (%p) \t| length %zu", \
                              (label), sv_fmt_args(sv), (void *)(sv).buf, (sv).length)

#define sv_assert_validity(sv) {                                                                          \
    SV_ASSERT((sv).buf != NULL, "Expected: non-NULL buf in string view, Received: %p", (void *)(sv).buf); \
    SV_ASSERT((sv).length >= 0, "Expected: positive length string view, Received: %zu", (sv).length);     \
  } while(0)

sv_t sv_from_buf(const char* buf, const size_t len)
{
  dbg(">> buf %.*s", (int)len, buf);
  SV_ASSERT(buf != NULL, "Expected: input buffer to be not-NULL, Received: %p", (void *)buf);

  sv_t sv = {
    .buf = buf,
    .length = len
  };

  sv_dbg("<<", sv);
  return sv;
}

sv_t sv_from_cstr(const char* str)
{
  dbg(">> cstr \"%s\"", str);

  sv_t sv = {
    .buf = str,
    .length = strlen(str)
  };

  sv_dbg("<<", sv);
  return sv;
}

bool sv_begins_with_word_buf(sv_t sv, const char *buf, const size_t len)
{
  sv_dbg("<<", sv);
  sv_assert_validity(sv);

  if (len > sv.length) {
    return false;
  } else if (len == sv.length) {
    return memcmp(sv.buf, buf, len) == 0;
  } else {
    return memcmp(sv.buf, buf, len) == 0 && isspace(sv.buf[len]);
  }

  return false;
}

bool sv_begins_with_word_cstr(sv_t sv, const char *str)
{
  return sv_begins_with_word_buf(sv, str, strlen(str));
}

bool sv_eq_cstr(sv_t sv, const char *str)
{
  sv_dbg(">>", sv);
  dbg(">> str %s len %zu", str, strlen(str));

  sv_assert_validity(sv);

  const size_t input_str_len = strlen(str);

  // empty strings
  if ((input_str_len + sv.length) == 0) {
    return true;
  }

  if (input_str_len == sv.length && *sv.buf == *str && memcmp(sv.buf, str, sv.length) == 0) {
    return true;
  }

  dbg("<<");
  return false;
}

bool sv_eq_sv(const sv_t sv1, const sv_t sv2)
{
  sv_dbg(">> sv1", sv1);
  sv_dbg(">> sv2", sv1);

  // empty strings
  if ((sv1.length + sv2.length) == 0) {
    return true;
  }

  if (sv1.length == sv2.length && *sv1.buf == *sv2.buf && memcmp(sv1.buf, sv2.buf, sv1.length) == 0) {
    return true;
  }

  return false;
}

bool sv_has_char(const sv_t sv, const char c)
{
  for (size_t i = 0; i < sv.length; i++) {
    if (sv.buf[i] == c) {
      return true;
    }
  }

  return false;
}

bool sv_is_empty(const sv_t sv)
{
  return sv.buf == NULL && sv.length == 0;
}

sv_t sv_trim_left(sv_t sv)
{
  sv_dbg(">>", sv);
  sv_assert_validity(sv);

  for (size_t i = 0; i < sv.length; i++) {
    if (!isspace(sv.buf[i])) {
      sv_t new_sv = { .buf = (sv.buf + i), .length = (sv.length - i) };
      sv_dbg("<<", new_sv);
      return new_sv;
    }
  }

  return sv;
  sv_dbg("<<", sv);
}

sv_t sv_trim_right(sv_t sv)
{
  sv_dbg(">>", sv);
  sv_assert_validity(sv);

  if (sv.length > 0) {
    for (size_t i = (sv.length - 1); i >= 0; i--) {
      if (!isspace(sv.buf[i])) {
        sv_t new_sv = { .buf = sv.buf, .length = i + 1 };
        sv_dbg("<<", new_sv);
        return new_sv;
      }
    }
  }

  sv_dbg("<<", sv);
  return sv;
}

sv_t sv_trim(sv_t sv)
{
  return sv_trim_right(sv_trim_left(sv));
}

/**
 * Example usage:
 * sv_t sv = sv_from_cstr("abc..123...000");
 * sv_t isv = {0};
 * do {
 *   isv = sv_split_by_char(&sv, '.');
 *   printf("ret: "SV_FMT" (%zu) remaining: "SV_FMT" (%zu)\n", sv_fmt_args(isv), isv.length, sv_fmt_args(sv), sv.length);
 * } while(isv.buf);
 *
 * Output: abc -> "" -> 123 -> "" -> "" -> 000 -> tombstone (which is (sv_t){ .buf = NULL, .length = 0 })
 */
sv_t sv_split_by_char(sv_t *const sv, char delim)
{
  sv_dbg(">>", *sv);
  dbg(">> delimiter '%c'", delim);
  sv_assert_validity(*sv);

  sv_t split = {0};

  // sv being split is empty. Return this tombstone where split.buf = NULL and split.length = 0
  if (sv->length <= 0) {
    return split;
  }

  split = (sv_t){ .buf = sv->buf, .length = 0 };
  bool found_delim = false;

  for (size_t i = 0; i < sv->length; i++) {
    split.length++;
    if (sv->buf[i] == delim) {
      found_delim = true;
      break;
    }
  }

  sv->buf = sv->buf + split.length;
  sv->length = sv->length - split.length;

  if (found_delim) {
    split.length--;
  }

  sv_dbg("<<", split);
  return split;
}

sv_t sv_split_from_idx(sv_t *const sv, const size_t from) /* including index (from) */
{
  sv_dbg(">>", *sv);
  dbg(">> split from index %zu", from);
  sv_assert_validity(*sv);

  sv_t split = { .buf = sv->buf, .length = 0 };

  if (sv->length != 0) {
    const size_t offset = from > sv->length ? sv->length : from;
    const size_t len = from > sv->length ? 0 : (sv->length - from);

    sv_t split = {
      .buf = sv->buf + offset,
      .length = len,
    };

    sv->length = offset;

    sv_dbg("<< updated input", *sv);
    sv_dbg("<< split chunk", split);
    return split;
  }

  sv_dbg("<<", split);
  return split;
}

sv_t sv_split_until_idx(sv_t *const sv, const size_t until) /* excluding index (until) */
{
  sv_dbg(">>", *sv);
  dbg(">> split until index %zu", until);
  sv_assert_validity(*sv);

  sv_t split = { .buf = sv->buf, .length = 0 };

  if (sv->length != 0) {
    const size_t len = until > sv->length ? sv->length : until;

    sv_t split = {
      .buf = sv->buf,
      .length = len
    };

    sv->buf = sv->buf + len;
    sv->length = sv->length - len;

    sv_dbg("<< updated input", *sv);
    sv_dbg("<< split chunk", split);
    return split;
  }

  sv_dbg("<<", split);
  return split;
}

#endif // ZDX_STRING_VIEW_IMPLEMENTATION
