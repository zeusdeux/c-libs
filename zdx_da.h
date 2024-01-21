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

#ifndef ZDX_DA_H_
#define ZDX_DA_H_

#include <assert.h>
#include <stdlib.h>
#include "./zdx_util.h"

#define DA_ASSERT assert // can be defined by consumer to not throw and log for example
#define DA_RESIZE_FACTOR 2 // double the dyn array capacity upon resize
#define DA_MIN_CAPACITY 8 // hold 8 elements by default

#define da_build_var_name(line, name, suffix) name ## _ ## line ## _ ## suffix
#define da_var(line, name, suffix) da_build_var_name(line, name, suffix)

#define da_el_sz(arr) sizeof((arr)[0])
#define da_arr_len(arr) sizeof((arr))/da_el_sz(arr)

#define dbg_da(label, da) dbg("%s length %zu\t\t\t| capacity %zu\t\t| items %p", \
                              label, (da)->length, (da)->capacity, (void *)(da)->items)

// guarded as the macros below use statement expressions which only gcc and clang support
#if __GNUC__ || __clang__
// Unsafe ops throw
#define da_pop(da)                                                      \
  ({                                                                    \
    DA_ASSERT((da)->length && "Cannot pop from empty container");       \
    (da)->items[--((da)->length)];                                      \
  })

// Unsafe ops throw
#define da_push__(da, els, reqd_cap, el_sz)                                                                                  \
  ({                                                                                                                         \
    dbg_da("input:\t", (da));                                                                                                \
    DA_ASSERT((da) != NULL && "Cannot operate on NULL dyn arr container");                                                   \
    DA_ASSERT((!(da)->capacity && !(da)->items && !(da)->length) || ((da)->capacity && (da)->items)                          \
              && "Invalid dyn arr container. Either all members must be zero or both capacity and items must be non-zero");  \
                                                                                                                             \
    dbg("input:\t element size %zu\t\t| required capacity %zu", el_sz, reqd_cap);                                            \
                                                                                                                             \
    if (((reqd_cap) + (da)->length) > (da)->capacity) {                                                                      \
      if((da)->capacity <= 0) {                                                                                              \
        (da)->capacity = DA_MIN_CAPACITY;                                                                                    \
      }                                                                                                                      \
      while((da)->capacity < ((reqd_cap) + (da)->length)) {                                                                  \
        (da)->capacity *= DA_RESIZE_FACTOR;                                                                                  \
      }                                                                                                                      \
      (da)->items = (da)->items ?                                                                                            \
        realloc((da)->items, (da)->capacity*sizeof(*(da)->items))                                                            \
        : calloc((da)->capacity, (el_sz));                                                                                   \
                                                                                                                             \
      DA_ASSERT((da)->items && "Allocation failed");                                                                         \
      dbg("resized:\t\t\t\t\t| new capacity %zu", (da)->capacity);                                                           \
    }                                                                                                                        \
                                                                                                                             \
    for(size_t  i = 0; i < (reqd_cap); i++)  {                                                                               \
        (da)->items[(da)->length++] = (els)[i];                                                                              \
    }                                                                                                                        \
                                                                                                                             \
    dbg_da("output:\t", da);                                                                                                 \
    (da)->length;                                                                                                            \
  })

#define da_push(da, ...) da_push__(da,                                                      \
                                  ((__typeof__((__VA_ARGS__))[]){__VA_ARGS__}),             \
                                  da_arr_len(((__typeof__((__VA_ARGS__))[]){__VA_ARGS__})), \
                                  da_el_sz(((__typeof__((__VA_ARGS__))[]){__VA_ARGS__})))


#define da_free(da) {                           \
    dbg_da("input:\t", (da));                   \
                                                \
    free((da)->items);                          \
    (da)->length = 0;                           \
    (da)->capacity = 0;                         \
    (da)->items = NULL;                         \
                                                \
    dbg_da("output:\t", (da));                  \
  } while(0);

#else // if not (__GNUC__ || __clang__)
#define da_push(da, ...) DA_ASSERT("dyn_array.h only works when compiled with gcc or clang")
#define da_pop(da, ...) DA_ASSERT("dyn_array.h only works when compiled with gcc or clang")
#define da_free(da) DA_ASSERT("dyn_array.h only works when compiled with gcc or clang")
#endif // __GNUC__ || __clang__

#endif // ZDX_DA_H_

#ifdef ZDX_DA_IMPLEMENTATION
// nothing needed here for now
#endif // ZDX_DA_IMPLEMENTATION
