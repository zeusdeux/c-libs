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
#ifndef ZDX_FILE_H_
#define ZDX_FILE_H_

#pragma GCC diagnostic error "-Wnonnull"
#pragma GCC diagnostic error "-Wnull-dereference"

#include <stddef.h>

typedef struct file_content {
  const char *err;
  size_t size; /* not off_t as size is set to the return value of fread which is size_t. Also off_t is a POSIX thing and size_t is std C */
  void *contents;
} fl_content_t;

#ifndef FL_API
#define FL_API
#endif // FL_API

#ifdef FL_ARENA_TYPE
FL_API fl_content_t fl_read_file(FL_ARENA_TYPE arena[const static 1], const char *restrict path, const char *restrict mode);
#else
FL_API fl_content_t fl_read_file(const char *restrict path, const char *restrict mode);
#endif // FL_ARENA_TYPE

FL_API void fc_deinit(fl_content_t fc[const static 1]);

#endif // ZDX_FILE_H_

// ----------------------------------------------------------------------------------------------------------------

#ifdef ZDX_FILE_IMPLEMENTATION

#include "./zdx_util.h"

#ifndef FL_ASSERT
#define FL_ASSERT assertm
#endif // FL_ASSERT

#define fc_dbg(label, fc) dbg("%s err %s \t| contents (%p) %s", \
                              (label), (fc).err, ((void *)(fc).contents), (char *)(fc).contents)

/* Guarded as unistd.h, sys/stat.h and friends are POSIX specific */
#if defined(__unix__) || defined(__unix) || defined(__linux__) || defined(__APPLE__) || defined(__MACH__)

#include <stdbool.h>
#include <stdlib.h> /* Needed for malloc, free, etc */
#include <string.h> /* Needed for strerror */
#include <limits.h> /* needed for PATH_MAX */
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#if defined(FL_ARENA_TYPE) && (!defined(FL_ALLOC) || !defined(FL_FREE))
_Static_assert(false, "When FL_ARENA_TYPE is defined, FL_ALLOC and FL_FREE must also be defined");
#endif

#ifndef FL_ALLOC
#define FL_ALLOC malloc
#endif // FL_ALLOC

#ifndef FL_FREE
#define FL_FREE free
#endif // FL_FREE


#ifdef FL_ARENA_TYPE
FL_API fl_content_t fl_read_file(FL_ARENA_TYPE arena[const static 1], const char *restrict path, const char *restrict mode)
#else
FL_API fl_content_t fl_read_file(const char *restrict path, const char *restrict mode)
#endif //FL_ARENA_TYPE
{
  dbg(">> path %s \t| mode %s", path, mode);

  /* guarded as it does a stack allocation */
#ifdef ZDX_TRACE_ENABLE
  char cwd[PATH_MAX] = {0};
  dbg(".. cwd %s", getcwd(cwd, sizeof(cwd)));
#endif

  /* init return type */
  fl_content_t fc = {0};

  /* open file */
  FILE *f = fopen(path, mode);

  if (f == NULL) {
    fclose(f);
    fc.err = strerror(errno);

    fc_dbg("<<", fc);
    return fc;
  }

  /* stat file for file size */
  struct stat s;

  if (fstat(fileno(f), &s) != 0) {
    fclose(f);
    fc.err = strerror(errno);

    fc_dbg("<<", fc);
    return fc;
  }

  const size_t sz = ((size_t)s.st_size + 1) * sizeof(char);

  /* read full file and close */
#ifdef FL_ARENA_TYPE
  char *contents_buf = FL_ALLOC(arena, sz);
#else
  char *contents_buf = FL_ALLOC(sz);
#endif

  size_t bytes_read = fread(contents_buf, sizeof(char), (size_t)s.st_size, f);

  fclose(f); /* safe to close as we have read contents into contents_buf */

  if (ferror(f)) {
    FL_FREE(contents_buf);
    fc.err = "Reading file failed";

    fc_dbg("<<", fc);
    return fc;
  }

  contents_buf[bytes_read] = '\0';

  fc.err = NULL;
  fc.size = bytes_read; /* as contents_buf is truncated with \0 at index bytes_read */
  fc.contents = (void *)contents_buf;

  fc_dbg("<<", fc);
  return fc;
}

FL_API void fc_deinit(fl_content_t fc[const static 1])
{
  fc_dbg(">>", *fc);

  FL_FREE(fc->contents);
  fc->err = NULL;
  fc->contents = NULL;
  fc->size = 0;

  fc_dbg("<<", *fc);
  return;
}

#elif defined(_WIN32) || defined(_WIN64)
_Static_assert(false, "zdx_file.h doesn't support windows yet");
#else
_Static_assert(false, "zdx_file.h doesn't support unknown OS yet");
/* Unsupported OSes */
#endif

#endif // ZDX_FILE_IMPLEMENTATION
