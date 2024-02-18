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
void *arena_calloc(arena_t *const ar, const size_t count, const size_t sz); /* check if mmap-ed mem is already zero-ed. It *might* be due to MAP_PRIVATE so do confirm it for osx/macos. It _is_ zero-ed on linux for example. */
void *arena_realloc(arena_t *const ar, void *ptr, const size_t sz); /* no idea what this should do tbh */

#endif // ZDX_SIMPLE_ARENA_H_

#ifdef ZDX_SIMPLE_ARENA_IMPLEMENTATION

#include <stddef.h>
#include <stdint.h>

/* Nothing that allocates should be included in non-debug builds */
#if defined(ZDX_TRACE_ENABLE) || defined(DEBUG)
#include <string.h> /* for memset in arena_create */
#include <stdio.h>
#include "./zdx_util.h"
#define ar_dbg(label, ar) dbg("%s arena %p \t| size %zu \t| offset %zu (%p) \t| err %s",      \
                              (label),                                                        \
                              (ar)->arena, (ar)->size,                                        \
                              (ar)->offset, (void *)((uintptr_t)(ar)->arena + (ar)->offset),  \
                              (ar)->err)
#else
#define ar_dbg(...)
#endif

typedef struct Arena {
  size_t size;
  size_t offset;
  void *arena;
  const char *err;
} arena_t;

/* gg windows */
#if defined(__unix__) || defined(__unix) || defined(__linux__) || defined(__APPLE__) || defined(__MACH__)

#include <unistd.h>
#include <sys/mman.h>

typedef enum {
  ARENA_ENOMEM = 0,
  ARENA_EINVAL,
  ARENA_EACQFAIL,
  ARENA_ERELFAIL

} arena_err_code_t;


static const char *arena_get_err_msg_(arena_err_code_t err_code)
{
  static const char *err_str[] = {
    "Arena cannot allocate memory",
    "Arena received invalid argument",
    "Arena failed to acquire memory",
    "Arena failed to release memory"
  };

  return err_str[err_code];
}

#define SA_DEFAULT_PAGE_SIZE_IF_UNDEF 4096

static inline size_t arena_round_up_to_page_size_(size_t sz)
{
  long page_size = sysconf(_SC_PAGESIZE);
  dbg(">> size %zu \t| system page size %ld", sz, page_size);

  if (page_size < 0) {
    dbg("<< sysconf failed");
    return 0;
  }
  /* this should never really happen but guarding just in case */
  page_size = page_size == 0 ? SA_DEFAULT_PAGE_SIZE_IF_UNDEF : page_size;

  size_t rounded_up_sz = sz < (size_t)page_size ? (size_t)page_size : ((sz / (size_t)page_size) + 1) * (size_t)page_size;

  dbg("<< rounded up size %zu", rounded_up_sz);
  return rounded_up_sz;
}

arena_t arena_create(size_t sz)
{
  dbg(">> requested size %zu", sz);

  arena_t ar = {0};

  if (!(sz = arena_round_up_to_page_size_(sz))) {
    ar.err = arena_get_err_msg_(ARENA_EINVAL);
    ar.arena = NULL;

    dbg("<< rounded arena size %zu \t| rounding failed", sz);
    return ar;
  }

  /* MAP_PRIVATE or MAP_SHARED must always be specified */
  /* from mmap man page - "Conforming applications must specify either MAP_PRIVATE or MAP_SHARED." */
  ar.arena = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

  if (ar.arena == MAP_FAILED) {
    ar.err = arena_get_err_msg_(ARENA_EACQFAIL);
    ar.arena = NULL;

    ar_dbg("<<", &ar);
    return ar;
  }

#if defined(DEBUG)
  memset(ar.arena, 0xcd, sz); // in debug mode memset whole arena to 0xcd to show up in debug tooling. It's also the value used by msvc I think
#endif

  ar.err = NULL;
  ar.size = sz;

  ar_dbg("<<", &ar);
  return ar;
}

bool arena_free(arena_t *const ar)
{
  ar_dbg(">>", ar);

  /* only unmap if arena has no pending errors and is non-NULL aka valid */
  if (!ar->err && ar->arena != NULL && munmap(ar->arena, ar->size) == 0) {
    ar->err = NULL;
    ar->arena = NULL;
    ar->size = 0;
    ar->offset = 0;
    ar_dbg("<<", ar);
    return true;
  }

  /* retain any previous error if an errored arena was passed in */
  if (!ar->err) {
    ar->err = ar->arena == NULL ? arena_get_err_msg_(ARENA_EINVAL) : arena_get_err_msg_(ARENA_ERELFAIL);
  }

  ar_dbg("<<", ar);
  return false;
}

/* this is to enable cross platform tests that assume an alignment */
#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT sizeof(max_align_t)
#endif

/*
 * Unaligned memory accesses occur when you try to read N bytes of data
 * starting from an address that is not evenly divisible by N (i.e. addr % N != 0).
 * For example, reading 4 bytes of data from address 0x10004 is fine,
 * but reading 4 bytes of data from address 0x10005 would be an unaligned memory access.
 *
 * Source: https://www.kernel.org/doc/html/v6.1/core-api/unaligned-memory-access.html#the-definition-of-an-unaligned-access
 *
 * Also, this is wasteful alignment as knowing proper alignment requires knowing the type
 * so that we can use _Alignof() for example. But this comes from the quote below -
 *
 * Pointers returned by allocation functions such as malloc are suitably aligned for any object,
 * which means they are aligned at least as strictly as max_align_t. max_align_t is usually
 * synonymous with the largest scalar type, which is long double on most platforms, and
 * its alignment requirement is either 8 or 16.
 *
 * Source: https://en.cppreference.com/w/c/types/max_align_t and https://stackoverflow.com/a/60133364
 */
static inline size_t arena_get_alignment_(size_t sz)
{
  /* this guard is so that we don't return an alignment value > DEFAULT_ALIGNMENT */
  if (sz < DEFAULT_ALIGNMENT) {
    /* the branches below return "natural alignment" for values belows DEFAULT_ALIGNMENT */
    /* the code below obviously assumes DEFAULT_ALIGNMENT to be equal to 8 */
    /* and that's fine as we are ok with wasted space if DEFAULT_ALIGNMENT > 8 */
    if (sz <= 1) {
      return sz;
    }
    if (sz <= 2) {
      return 2;
    }
    if (sz <= 4) {
      return 4;
    }
  }
  return DEFAULT_ALIGNMENT;
}

/* aligned arena allocator */
void *arena_alloc(arena_t *const ar, const size_t sz)
{
  ar_dbg(">>", ar);
  dbg(">> requested size %zu", sz);

  /* validity check */
  if (ar->err || ar->arena == NULL || ar->size <= 0 || ar->offset > ar->size || ar->offset < 0) {
    ar->err = ar->err != NULL ? ar->err : arena_get_err_msg_(ARENA_EINVAL);

    ar_dbg("<<", ar);
    return NULL;
  }

  uintptr_t ptr = (uintptr_t)ar->arena + ar->offset;

  if (sz > 0) {
    size_t alignment = arena_get_alignment_(sz);
    size_t remainder = 0;

    if ((remainder = ptr % alignment) != 0) {
      size_t padding = alignment - remainder;
      ptr += padding;
    }
    /* not using ptrdiff_t as ptr is guaranteed to be greater than ar->arena due to checks and ptr assignment above */
    size_t ptr_offset = ptr - (uintptr_t)ar->arena;
    size_t remaining = ar->size - ptr_offset;
    dbg("<< padded ptr %p", (void *)ptr);

    /* non-resizeable arena hence we return NULL to signal error */
    if (sz > remaining) {
      ar->err = arena_get_err_msg_(ARENA_ENOMEM);

      dbg("<< remaining %zu", remaining);
      ar_dbg("<<", ar);
      return NULL;
    }

    /* we need to point to address after the one we are returning in ptr */
    ar->offset = ptr_offset + sz;
  }

  ar_dbg("<<", ar);
  dbg("<< ptr %p", (void *)ptr);
  return (void *)ptr;
}
/* check if mmap-ed mem is already zero-ed. It *might* be due to MAP_PRIVATE so do confirm it for osx/macos. It _is_ zero-ed on linux for example. */
void *arena_calloc(arena_t *const ar, const size_t count, const size_t sz)
{
  dbg(">> count %zu \t| size %zu", count, sz);
  size_t total_size = count * sz;
  void *ptr = arena_alloc(ar, total_size);
  // not zeroing memory as #if defined(__unix__) || defined(__unix) || defined(__linux__) || defined(__APPLE__) || defined(__MACH__) return zero-filled
  // memory on MAP_ANONYMOUS
  dbg("<< ptr %p", ptr);
  return ptr;
}

#elif defined(_WIN32) || defined(_WIN64)
/* No support for windows yet */
#else
/* Unsupported OSes */
#endif

#endif // ZDX_SIMPLE_ARENA_IMPLEMENTATION
