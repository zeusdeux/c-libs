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

/* defines symbols size_t, NULL, malloc, realloc, etc needed by macros and structs declarations */
#include <stdlib.h>
/* needed for usage of int64_t, etc in forward declarations below */
#include <stdint.h>
/* needed for macros used in the header section */
#include <stdbool.h>
#include "./zdx_util.h"

/**
 * This lib should contain string builder functionality (with interfaces for buf as well as cstr)
 * and string view functionality. It should also contain convenience methods for reading and
 * writing full files, read and write to files or buffers by line, etc. Maybe even replacements
 * for splitting a string by token (as strtok is poo) and lazily getting parts back, etc.
 */
#pragma GCC diagnostic error "-Wnonnull"
#pragma GCC diagnostic error "-Wnull-dereference"

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


/* ---- GAP BUFFER ---- */

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
void gb_insert_cstr(gb_t gb[const static 1], const char cstr[static 1]);
void gb_delete_chars(gb_t gb[const static 1], const int64_t count); // count +ve -> delete, count -ve -> backspace
size_t gb_get_cursor(gb_t gb[const static 1]);
char *gb_buf_as_cstr(const gb_t gb[const static 1]);


/* --- FILE HELPERS ---- */

#ifndef FL_MALLOC
#define FL_MALLOC malloc
#endif // FL_MALLOC

#ifndef FL_FREE
#define FL_FREE free
#endif // FL_FREE


typedef struct file_content {
  bool is_valid;
  char *err_msg;
  void *contents;
} fl_content_t;

/*
 * The arguments to fl_read_full_file are the same as fopen.
 * It returns a char* that must be freed by the caller
 */
fl_content_t fl_read_file_str(const char *restrict path, const char *restrict mode);

#endif // ZDX_STR_H_


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
  return;
}


/* ---- GAP BUFFER IMPLEMENTATION ---- */

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
#define gb_len_with_gap(gb) ((gb)->length + gb_gap_len(gb))

static char *gb_buf_as_dbg_cstr(const gb_t gb[const static 1])
{
  gb_assert_validity(gb);

  size_t buf_len_with_gap = gb_len_with_gap(gb);
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

  size_t buf_len_with_gap = gb_len_with_gap(gb);

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
    memmove(dst, src, gb->length - gb->gap_start_);

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

/* count -ve -> backspace, count +ve -> delete */
void gb_delete_chars(gb_t gb[const static 1], const int64_t count)
{
  gb_dbg(">>", gb);
  gb_assert_validity(gb);

  /* delete */
  if (count > 0) {
    int64_t new_gap_end = gb->gap_end_ + count;
    new_gap_end = new_gap_end > ((int64_t) gb_len_with_gap(gb)) ? gb_len_with_gap(gb) : new_gap_end;

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


/* FILE HELPERS IMPLEMENTATION */

/* Guarded as unistd.h, sys/stat.h and friends are POSIX specific */
#if defined(__unix__) || defined(__unix) || defined(__linux__) || defined(__APPLE__) || defined(__MACH__)

/* needed for PATH_MAX */
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#define fc_dbg(label, fc) dbg("%s valid %d \t| err %s \t| contents (%p) %s", \
                              (label), (fc).is_valid, (fc).err_msg, ((void *)(fc).contents), (char *)(fc).contents)

fl_content_t fl_read_file_str(const char *restrict path, const char *restrict mode)
{
  dbg(">> path %s \t| mode %s", path, mode);

#ifdef ZDX_TRACE_ENABLE
  char cwd[PATH_MAX] = {0};
  dbg(">> cwd %s", getcwd(cwd, sizeof(cwd)));
#endif

  /* init return type */
  fl_content_t fc = {
    .is_valid = false,
    .err_msg = NULL,
    .contents = NULL
  };

  /* open file */
  FILE *f = fopen(path, mode);

  if (f == NULL) {
    fc.is_valid = false;
    fc_dbg("<<", fc);
    return fc;
  }

  /* stat file for file size */
  struct stat s;

  if (fstat(fileno(f), &s) != 0) {
    fc.is_valid = false;
    fc.err_msg = strerror(errno);
    fc_dbg("<<", fc);
    return fc;
  }

  /* read full file and close */
  char *contents_buf = FL_MALLOC((s.st_size + 1) * sizeof(char));
  size_t bytes_read = fread(contents_buf, sizeof(char), s.st_size, f);
  fclose(f);

  if (ferror(f)) {
    fc.is_valid = false;
    fc.err_msg = "Reading file failed";
    fc_dbg("<<", fc);
    return fc;
  }

  contents_buf[bytes_read] = '\0';

  fc.is_valid = true;
  fc.err_msg = false;
  fc.contents = (void *)contents_buf;

  fc_dbg("<<", fc);
  return fc;
}

void fc_deinit(fl_content_t *fc)
{
  fc_dbg(">>", *fc);

  FL_FREE(fc->contents);
  fc->contents = NULL;
  fc->is_valid = false;
  fc->err_msg = NULL;

  fc_dbg("<<", *fc);
  return;
}

#elif defined(_WIN32) || defined(_WIN64)
/* No support for windows yet */
#else
/* Unsupported OSes */
#endif

#endif // ZDX_STR_IMPLEMENTATION
