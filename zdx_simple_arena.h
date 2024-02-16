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
/*
 * A _very_ simple mmap based bump/region/arena allocator for my own use
 */
#ifndef ZDX_SIMPLE_ARENA_H_
#define ZDX_SIMPLE_ARENA_H_

#include <stdbool.h>

#pragma GCC diagnostic error "-Wnonnull"
#pragma GCC diagnostic error "-Wnull-dereference"
#pragma GCC diagnostic error "-Wsign-conversion"

typedef struct Arena arena_t;

arena_t arena_create(const size_t sz);
bool arena_free(arena_t *const ar);
void *arena_alloc(arena_t *const ar, const size_t sz); /* make sure to align addresses */
void *arena_realloc(arena_t *const ar, void *ptr, const size_t sz); /* no idea what this should do tbh */
void *arena_calloc(arena_t *const ar, const size_t count, const size_t sz); /* check if mmap-ed mem is already zero-ed. It *might* be due to MAP_PRIVATE so do confirm it for osx/macos. It _is_ zero-ed on linux for example. */

#endif // ZDX_SIMPLE_ARENA_H_

#ifdef ZDX_SIMPLE_ARENA_IMPLEMENTATION

#include <stddef.h>
#include <stdint.h>

/* Nothing that allocates should be included in non-debug builds */
#if defined(ZDX_TRACE_ENABLE) || defined(DEBUG)
#include <stdio.h>
#include "./zdx_util.h"
#define ar_dbg(label, ar) dbg("%s arena %p \t| size %zu \t| offset %zu \t| valid %d \t", \
                              (label), (ar)->arena, (ar)->size, (ar)->offset, (ar)->is_valid)
#else
#define ar_dbg(...)
#endif

typedef struct Arena {
  void *arena;
  size_t size;
  size_t offset;
  bool is_valid;
} arena_t;

/* gg windows */
#if defined(__unix__) || defined(__unix) || defined(__linux__) || defined(__APPLE__) || defined(__MACH__)

#include <unistd.h>
#include <sys/mman.h>

arena_t arena_create(size_t sz)
{
  arena_t ar = {0};
  long page_size = sysconf(_SC_PAGESIZE);

  sz = sz < (size_t)page_size ? (size_t)page_size : ((sz / (size_t)page_size) + 1) * (size_t)page_size;
  dbg(">> size %zu \t| sys page size %ld", sz, page_size);

  /* MAP_PRIVATE or MAP_SHARED must always be specified */
  /* from mmap man page - "Conforming applications must specify either MAP_PRIVATE or MAP_SHARED." */
  ar.arena = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

  if (ar.arena == MAP_FAILED) {
    ar.is_valid = false;
    ar.arena = NULL;
    ar_dbg("<<", &ar);
    return ar;
  }

  ar.is_valid = true;
  ar.size = sz;

  ar_dbg("<<", &ar);
  return ar;
}

bool arena_free(arena_t *const ar)
{
  ar_dbg(">>", ar);

  if (ar->arena != NULL && munmap(ar->arena, ar->size) == 0) {
    ar->arena = NULL;
    ar->size = 0;
    ar->offset = 0;
    ar->is_valid = false;
    dbg("<< arena freed? true");
    return true;
  }

  dbg("<< arena freed? false");
  return false;
}

#elif defined(_WIN32) || defined(_WIN64)
/* No support for windows yet */
#else
/* Unsupported OSes */
#endif

#endif // ZDX_SIMPLE_ARENA_IMPLEMENTATION
