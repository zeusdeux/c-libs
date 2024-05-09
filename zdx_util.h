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

#ifndef ZDX_UTIL_H_
#define ZDX_UTIL_H_

#include <stdio.h> /* needed for fprintf, etc */
#include <stdlib.h> /* needed for abort(), exit(), EXIT_FAILURE macro etc */

#define KB * 1024
#define MB KB * 1024

#define zdx_min(a, b) ((a) < (b) ? (a) : (b))
#define zdx_max(a, b) ((a) > (b) ? (a) : (b))

// Arg extraction macros
#define zdx_last_arg(...) (__VA_ARGS__)
#define zdx_first_arg(first, ...) (first)

// Array macros. Check for false-y values of (arr) BEFORE calling these. Use zdx_truthy() for e.g.
#define zdx_el_sz(arr) sizeof((arr)[0])
#define zdx_arr_len(arr) sizeof((arr)) / zdx_el_sz(arr)

#define assertm(cond, ...)                                                                                \
  (cond) ?                                                                                                \
         ((void)0)                                                                                        \
       : (fprintf(stderr, "%s:%d:\t[%s] FAILED ASSERTION: %s => ", __FILE__, __LINE__, __func__, #cond),  \
          fprintf(stderr, "\n\tREASON: "),                                                                \
          fprintf(stderr, __VA_ARGS__),                                                                   \
          fprintf(stderr, "\n"),                                                                          \
          abort())

#define bail(...) do {                          \
    log(L_ERROR, __VA_ARGS__);                  \
    exit(EXIT_FAILURE);                         \
  } while(0)

#if defined(ZDX_TRACE_ENABLE)
#define dbg(...) do {                                               \
    fprintf(stderr, "%s:%d:\t[%s] ", __FILE__, __LINE__, __func__); \
    fprintf(stderr, __VA_ARGS__);                                   \
    fprintf(stderr, "\n");                                          \
  } while(0)
#else
#define dbg(...)
#endif // ZDX_TRACE_ENABLE

typedef enum {
  L_ERROR = 1,
  L_WARN,
  L_INFO
} ZDX_LOG_LEVEL;

#ifdef ZDX_LOGS_DISABLE
#define log(level, ...)
#else
static const char *ZDX_LOG_LEVEL_STR[] = {
  "",
  "ERROR",
  "WARN",
  "INFO"
};

#define log(level, ...) do {                                    \
    fprintf(stderr, "%s:%d: ", __FILE__, __LINE__);             \
    fprintf(stderr, "[%s] ", ZDX_LOG_LEVEL_STR[(level)]);       \
    fprintf(stderr, __VA_ARGS__);                               \
    fprintf(stderr, "\n");                                      \
  } while(0)
#endif // ZDX_LOGS_DISABLE

#endif // ZDX_UTIL_H_
