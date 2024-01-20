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

#ifndef ZDX_UTIL_H_
#define ZDX_UTIL_H_

#include <stdio.h>
#include <stdlib.h>

#define bail(...) {                             \
    dbg(__VA_ARGS__);                           \
    exit(1);                                    \
  }

#ifdef ZDX_TRACE_ENABLE
#define dbg(...) {                                                      \
    fprintf (stderr, "%s:%d:\t[%s] ", __FILE__, __LINE__, __func__);    \
    fprintf (stderr, __VA_ARGS__);                                      \
    fprintf (stderr, "\n");                                             \
  }
#else
#define dbg(...) {}
#endif // ZDX_TRACE_ENABLE

#ifdef NZDX_LOG_ENABLE
#define log(level, ...) {}
#else
typedef enum {
  L_ERROR = 1,
  L_WARN,
  L_INFO
} ZDX_LOG_LEVEL;

static const char *ZDX_LOG_LEVEL_STR[] = {
  "",
  "ERROR",
  "WARN",
  "INFO"
};

#define log(level, ...) {                                       \
    fprintf (stderr, "%s:%d: ", __FILE__, __LINE__);            \
    fprintf (stderr, "[%s] ", ZDX_LOG_LEVEL_STR[level]);        \
    fprintf (stderr, __VA_ARGS__);                              \
    fprintf (stderr, "\n");                                     \
  }
#endif // NZDX_LOG_ENABLE
#endif // ZDX_UTIL_H_