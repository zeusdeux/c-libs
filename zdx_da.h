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

#ifndef ZDX_DA_H_
#define ZDX_DA_H_

#include <stdlib.h>
#include "./zdx_util.h"

#ifndef DA_ASSERT
#define DA_ASSERT assertm // can be defined by consumer to not throw and log for example
#endif // DA_ASSERT

#ifndef DA_REALLOC
#define DA_REALLOC realloc
#endif // DA_REALLOC

#ifndef DA_FREE
#define DA_FREE free
#endif // DA_FREE


#ifndef DA_RESIZE_FACTOR
#define DA_RESIZE_FACTOR 2 // double the dyn array capacity upon resize
#endif // DA_RESIZE_FACTOR

#ifndef DA_MIN_CAPACITY
#define DA_MIN_CAPACITY 8 // hold 8 elements by default
#endif // DA_MIN_CAPACITY

#define dbg_da(label, da) dbg("%s length %zu\t\t\t| capacity %zu\t\t| items %p", \
                              (label), (da)->length, (da)->capacity, (void *)(da)->items)

// guarded as the macros below use statement expressions which only gcc and clang support
#if __GNUC__ || __clang__
// Unsafe ops throw
#define da_pop(da)                                                                      \
  _Pragma("GCC diagnostic push")                                                        \
  _Pragma("GCC diagnostic ignored \"-Wgnu-statement-expression-from-macro-expansion\"") \
  ({                                                                                    \
    DA_ASSERT((da)->length, "[zdx_da] Cannot pop from empty container");                \
    (da)->items[--((da)->length)];                                                      \
  })                                                                                    \
  _Pragma("GCC diagnostic pop")

// Unsafe ops throw
#define da_push_(da, els, reqd_cap)                                                                                       \
  _Pragma("GCC diagnostic push")                                                                                          \
  _Pragma("GCC diagnostic ignored \"-Wgnu-statement-expression-from-macro-expansion\"")                                   \
  ({                                                                                                                      \
    dbg_da(">>", da);                                                                                                     \
    DA_ASSERT((da) != NULL, "[zdx_da] Cannot operate on NULL container");                                                 \
    DA_ASSERT((!(da)->capacity && !(da)->items && !(da)->length) || ((da)->capacity && (da)->items),                      \
              "[zdx_da] Invalid container. Either all members must be zero or both capacity and items must be non-zero"); \
    DA_ASSERT((els) != NULL, "[zdx_da] Pushing no elements is invalid");                                                  \
    DA_ASSERT((reqd_cap) > 0, "[zdx_da] Pushing no elements is invalid");                                                 \
                                                                                                                          \
    dbg(">>\t\t\t\t| required capacity %zu", reqd_cap);                                                                   \
                                                                                                                          \
    if (((reqd_cap) + (da)->length) > (da)->capacity) {                                                                   \
      if((da)->capacity <= 0) {                                                                                           \
        (da)->capacity = DA_MIN_CAPACITY;                                                                                 \
      }                                                                                                                   \
      while((da)->capacity < ((reqd_cap) + (da)->length)) {                                                               \
        (da)->capacity *= DA_RESIZE_FACTOR;                                                                               \
      }                                                                                                                   \
      (da)->items = DA_REALLOC((da)->items, (da)->capacity*sizeof(*(da)->items));                                         \
      DA_ASSERT((da)->items, "[zdx_da] Allocation failed");                                                               \
      dbg("++ resized\t\t\t| new capacity %zu", (da)->capacity);                                                          \
    }                                                                                                                     \
                                                                                                                          \
    for(size_t i = 0; i < (reqd_cap); i++)  {                                                                             \
      (da)->items[(da)->length++] = (els)[i];                                                                             \
    }                                                                                                                     \
                                                                                                                          \
    dbg_da("<<", da);                                                                                                     \
    (da)->length;                                                                                                         \
  })                                                                                                                      \
  _Pragma("GCC diagnostic pop")

#define da_push(da, ...) da_push_(da,                                            \
                                  ((__typeof__((__VA_ARGS__))[]){__VA_ARGS__}),  \
                                  zdx_arr_len(((__typeof__((__VA_ARGS__))[]){__VA_ARGS__})));

#define da_deinit(da)                           \
  do {                                          \
    dbg_da(">>", da);                           \
                                                \
    DA_FREE((da)->items);                       \
    (da)->length = 0;                           \
    (da)->capacity = 0;                         \
    (da)->items = NULL;                         \
                                                \
    dbg_da("<<", da);                           \
  } while(0);
#else // if not (__GNUC__ || __clang__)
#define da_push(da, ...) DA_ASSERT(false, "[zdx_da] zdx_da.h only works when compiled with gcc or clang")
#define da_pop(da, ...) DA_ASSERT(false, "[zdx_da] zdx_da.h only works when compiled with gcc or clang")
#define da_deinit(da) DA_ASSERT(false, "[zdx_da] zdx_da.h only works when compiled with gcc or clang")
#endif // __GNUC__ || __clang__

#endif // ZDX_DA_H_

#ifdef ZDX_DA_IMPLEMENTATION
// nothing needed here for now
#endif // ZDX_DA_IMPLEMENTATION
