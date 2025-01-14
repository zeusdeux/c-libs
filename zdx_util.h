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
 * NOTE:
 *
 * THIS LIBRARY SHOULD NEVER DEPEND ON ANY OTHER EXCEPT STANDARD LIBS!
 * IT SHOULD BE FULLY SELF-SUFFICIENT TO BE ATOMIC!
 *   Serving these properties lets us rely on this library as a
 *   building block library.
 *
 * Keep it simple!
 */

// C std says that each #include <assert.h> should respect the value of NDEBUG macro
// at the moment of inclusion. Therefore, to retain the same (in)sane semantics, we
// declare our assertm macro before out multiple include guard (i.e., the #ifndef ZDX_UTIL_H_)
// POSIX only. GG windows
#if defined(__unix__) || defined(__unix) || defined(__linux__) || defined(__APPLE__) || defined(__MACH__)
#include <stdio.h> /* needed for fprintf, etc */
#include <stdlib.h> /* needed for abort(), exit(), EXIT_FAILURE macro etc */

// this is because we allow multiple imports of zdx_util.h to have differing behavior
// for assertm() like assert from assert.h
#undef assertm

#if !defined(NDEBUG)
#define assertm(cond, ...)                                                                               \
  (cond) ?                                                                                               \
         ((void)0)                                                                                       \
       : (fprintf(stderr, "%s:%d:\t[%s] FAILED ASSERTION: %s => ", __FILE__, __LINE__, __func__, #cond), \
          fprintf(stderr, "\n\tREASON: "),                                                               \
          fprintf(stderr, __VA_ARGS__),                                                                  \
          fprintf(stderr, "\n"),                                                                         \
          abort())
#else
#define assertm(...) ((void)0)
#endif // NDEBUG
#endif // POSIX


#ifndef ZDX_UTIL_H_
#define ZDX_UTIL_H_

#define KB * 1024
#define MB KB * 1024

#define zdx_min(a, b) ((a) < (b) ? (a) : (b))
#define zdx_max(a, b) ((a) > (b) ? (a) : (b))

// Arg extraction macros
#define zdx_last_arg(...)         (__VA_ARGS__)
#define zdx_first_arg(first, ...) (first)

// Array macros. Check for false-y values of (arr) BEFORE calling these as the dereference of NULL == seg fault
#define zdx_el_sz(arr)   sizeof(*(arr))
#define zdx_arr_len(arr) sizeof((arr)) / zdx_el_sz(arr)

// Version is the lowest for gcc and clang that supports both _Generic from C11 and
// statement expressions as used by the CLOSESTPOWEROF2 macro
#if (__GNUC__ && __GNUC__ >= 5) || (__clang__ && __clang_major__ >= 5)

#include <stdint.h> /* needed for uint<bits>_t types such as uint32_t, etc */

// TODO(mudit): Remove POPCOUNTG and CLZG macros once __builtin_popcountg and __builtin_clzg
//              are widely available on default compilers for platforms.

#define POPCOUNTG(n) _Generic((n),                                      \
                              unsigned int: __builtin_popcount,         \
                              unsigned long: __builtin_popcountl,       \
                              unsigned long long: __builtin_popcountll  \
                              )((n))

// DO NOT pass n == 0 to this as we aren't using the second param of
// __builtin_clz{,l,ll} to return a fallback value when n == 0
#define NONZERO_CLZG(n) _Generic((n),                                   \
                                 unsigned int: __builtin_clz,           \
                                 unsigned long: __builtin_clzl,         \
                                 unsigned long long: __builtin_clzll    \
                                 )((n))

#define CLOSESTPOWEROF2(n)                                                              \
  _Pragma("GCC diagnostic push")                                                        \
  _Pragma("GCC diagnostic ignored \"-Wgnu-statement-expression-from-macro-expansion\"") \
  ({                                                                                    \
    uint8_t bits = sizeof(__typeof__((n))) * 8;                                         \
                                                                                        \
    __typeof__(n) result = n;                                                           \
    uint8_t popcnt = POPCOUNTG((n));                                                    \
                                                                                        \
    if (popcnt > 1) {                                                                   \
      uint8_t clz = NONZERO_CLZG(n);                                                    \
                                                                                        \
      result = (__typeof__(n))1 << (bits - clz);                                        \
    }                                                                                   \
                                                                                        \
    result;                                                                             \
  })                                                                                    \
  _Pragma("GCC diagnostic pop")
#else
_Static_assert(0, "Unsupported compiler. Cannot define POPCOUNTG, NONZERO_CLZG and CLOSESTPOWEROF2");
#endif // gnu and clang version check ifdef


// POSIX only for all the helper macros below. GG windows
#if defined(__unix__) || defined(__unix) || defined(__linux__) || defined(__APPLE__) || defined(__MACH__)

#ifdef ZDX_PROF_ENABLE
#include <time.h>

#define PROF_START(name) struct timespec begin_##name = {0};    \
  clock_gettime(CLOCK_MONOTONIC, &begin_##name)

#define PROF_END(name)   struct timespec end_##name = {0};                           \
  clock_gettime(CLOCK_MONOTONIC, &end_##name);                                       \
  const double b_##name = (double)begin_##name.tv_sec + begin_##name.tv_nsec * 1e-9; \
  const double e_##name = (double)end_##name.tv_sec + end_##name.tv_nsec * 1e-9;     \
  const double elapsed_time_##name = e_##name - b_##name;                            \
  fprintf(stderr, "PROF(" #name "): %0.9lfsecs\n", elapsed_time_##name)

#else
#define PROF_START(name)
#define PROF_END(name)
#endif // ZDX_PROF_ENABLE


#if defined(ZDX_TRACE_ENABLE)
#define dbg(...) do {                                               \
    fprintf(stderr, "%s:%d:\t[%s] ", __FILE__, __LINE__, __func__); \
    fprintf(stderr, __VA_ARGS__);                                   \
    fprintf(stderr, "\n");                                          \
  } while(0)
#else
#define dbg(...)
#endif // ZDX_TRACE_ENABLE

#ifdef DEBUG
#define bail(...) do {                                              \
    fprintf(stderr, "%s:%d:\t[%s] ", __FILE__, __LINE__, __func__); \
    fprintf(stderr, __VA_ARGS__);                                   \
    fprintf(stderr, "\n");                                          \
    exit(EXIT_FAILURE);                                             \
  } while(0)
#else
#define bail(...) do {                          \
    fprintf(stderr, __VA_ARGS__);               \
    fprintf(stderr, "\n");                      \
    exit(EXIT_FAILURE);                         \
  } while(0)
#endif // DEBUG

typedef enum {
  L_ERROR = 0,
  L_WARN,
  L_INFO,
  L_COUNT,
} ZDX_LOG_LEVEL;

#ifdef ZDX_LOGS_DISABLE
#define log(level, ...)
#else
static const char *ZDX_LOG_LEVEL_STR[L_COUNT] = {
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

#endif // POSIX
#endif // ZDX_UTIL_H_
