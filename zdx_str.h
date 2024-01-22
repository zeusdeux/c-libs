#ifndef ZDX_STR_H_
#define ZDX_STR_H_

/**
 * This lib should contain string builder functionality (with interfaces for buf as well as cstr)
 * and string view functionality. It should also contain convenience methods for reading and
 * writing full files, read and write to files or buffers by line, etc. Maybe even replacements
 * for splitting a string by token (as strtok is poo) and lazily getting parts back, etc.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "./zdx_util.h"
#include "./zdx_da.h"

typedef struct {
  const char *str;
  size_t length;
} sb_t;

#define sb_first_arg(first, ...) (first)

#define sb_concat(sb, arr)                      \
  sb_append_cstrs_((sb),                        \
                   da_arr_len((arr)),           \
                   (arr))
#define sb_append(sb, ...)                                                    \
  sb_append_cstrs_((sb),                                                      \
                   da_arr_len(((__typeof__((__VA_ARGS__))[]){__VA_ARGS__})),  \
                   sb_first_arg(__VA_ARGS__, "<FILLER>") ? ((const char **)(__typeof__((__VA_ARGS__))[]){__VA_ARGS__}) : NULL)

void sb_append_cstrs_(sb_t sb[const static 1], const size_t count, const char *cstrs[static count]);
// same as sb_free(sb_t *const sb) but the [static 1] enforces a pointer non-null check during compile
void sb_free(sb_t sb[const static 1]);

#endif // ZDX_STR_H_

#ifdef ZDX_STR_IMPLEMENTATION

void sb_append_cstrs_(sb_t sb[const static 1], const size_t count, const char *cstrs[static count])
{
  assertm(sb != NULL, "[zdx_str] Expected: valid string builder instance, Received: %p", (void *)sb);
  assertm(count > 0, "[zdx_str] Expected: number of cstrings to insert to be > 0, Received: %zu", count);
  assertm(cstrs != NULL, "[zdx_str] Expected: array of cstring to insert to be non-NULL, Received: %p", (void *)cstrs);
  dbg("input:\tcount %zu\t| cstrs %p", count, (void *)cstrs);
}

void sb_free(sb_t  sb[const static 1])
{
  free((void *)sb->str);
  sb->str = NULL;
  sb->length = 0;
}

#endif // ZDX_STR_IMPLEMENTATION
