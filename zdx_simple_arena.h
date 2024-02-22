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
bool arena_reset(arena_t *const ar);
void *arena_alloc(arena_t *const ar, const size_t sz);
void *arena_calloc(arena_t *const ar, const size_t count, const size_t sz);
void *arena_realloc(arena_t *const ar, void *ptr, const size_t old_sz, const size_t sz); /* no idea what this should do tbh */

#endif // ZDX_SIMPLE_ARENA_H_

#ifdef ZDX_SIMPLE_ARENA_IMPLEMENTATION

#include <stddef.h>
#include <stdint.h>
#include <string.h> /* for memset in arena_create and memcpy in arena_realloc */

/* Nothing that allocates should be included in non-debug builds */
#if defined(ZDX_TRACE_ENABLE) || defined(DEBUG)
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
  /* This is to communicate errors outwards rather than for use by fns in this lib.
   * Functions in this lib should do their own validations before proceeding.
   * If their validations fail, then they should overwrite this with their own error.
   * The user can therefore rely on "err" always pointing to the failure of the
   * last function call. It is up to the user of this lib to always check this property
   * before making calls to other functions in this lib if they want the failure reason
   * for their last call.
   */
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
    "Arena invalid or invalid argument",
    "Arena failed to acquire memory (check errno)",
    "Arena failed to release memory (check errno)"
  };

  return err_str[err_code];
}

#define SA_DEFAULT_PAGE_SIZE_IF_UNDEF 4096

/* this function should never fail, and always return a non-zero +ve value as mmap/munmap with size <= 0 will always fail */
static inline size_t arena_round_up_to_page_size_(size_t sz)
{
  long page_size = sysconf(_SC_PAGESIZE);
  dbg(">> size %zu \t| system page size %ld", sz, page_size);

  if (page_size < 0) {
    dbg("!! sysconf failed. Will round up to %d", SA_DEFAULT_PAGE_SIZE_IF_UNDEF);
  }

  /* round up to SA_DEFAULT_PAGE_SIZE_IF_UNDEF if sysconf failed (-1) or returned 0 for some reason */
  page_size = page_size <= 0 ? SA_DEFAULT_PAGE_SIZE_IF_UNDEF : page_size;

  /* round up requested size of a multiple of page_size */
  size_t rounded_up_sz = sz < (size_t)page_size ? (size_t)page_size : ((sz / (size_t)page_size) + 1) * (size_t)page_size;

  dbg("<< rounded up size %zu", rounded_up_sz);
  return rounded_up_sz;
}

/*
 * DESCRIPTION
 *
 * This function creates an arena with a backing memory of *atleast* size bytes.
 * The size parameter is always rounded up to page size of the machine. This means you
 * could get an arena larger than you requested but never smaller.
 *
 * RETURN VALUES
 *
 * If creation of the backing memory was successful, then the newly created arena is
 * returned *by value* and *not* as a pointer as it can be trivially passed around
 * via the stack. Automatic allocation is your friend!
 * If there was an error, then an empty arena is created that has no backing memory
 * and the "err" property is set with the error message. It can be passed to arena_free().
 *
 * NOTES
 *
 * If DEBUG is defined, then the whole arena is also memset to 0xcd to help with debugging.
 */
arena_t arena_create(size_t sz)
{
  dbg(">> requested size %zu", sz);

  arena_t ar = {0};
  sz = arena_round_up_to_page_size_(sz);

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

  ar.size = sz;

  ar_dbg("<<", &ar);
  return ar;
}

/*
 * DESCRIPTION
 *
 * This function frees the memory backing the arena that's passed in.
 * arena_free() is a safe function to call even with an "invalid" arena that
 * for e.g., has "err" property set or is wrecked in some way so that
 * users aren't stuck with invalid arenas and with no way to clear them.
 * Also, from my tests on aarch64, munmap(NULL, size > 0) seems to always succeed.
 * Whereas, munmap(NULL, 0) always fails due to size being 0.
 * But that could be implementation specific.
 * Either way, munmap() failures are handled so we are good.
 *
 * RETURN VALUES
 *
 * true is returned if the backing memory of the arena was freed.
 * false is returned if an error occured and the "err" property is set to the
 * error message.
 */
bool arena_free(arena_t *const ar)
{
  ar_dbg(">>", ar);

  /* we leave error handling of NULL addr or size <= 0 to munmap */
  if (munmap(ar->arena, ar->size) < 0) {
    ar->err = arena_get_err_msg_(ARENA_ERELFAIL);

    ar_dbg("<<", ar);
    return false;
  }

  ar->size = 0;
  ar->offset = 0;
  ar->arena = NULL;
  ar->err = NULL;

  ar_dbg("<<", ar);
  return true;
}

/*
 * DESCRIPTION
 *
 * This function resets an arena without deallocating the backing memory to allow
 * for reuse of the arena without the overhead of freeing and reallocation. It also
 * clears the "err" property of the arena. It always suceeds and can therefore, be
 * safely called without any guards.
 *
 * RETURN VALUES
 *
 * true is always returned.
 *
 * NOTES
 *
 * Given that this function clears the "err" property, if the arena *did* have an error
 * before this was called, the error will be lost upon returning from this function.
 */
bool arena_reset(arena_t *const ar)
{
  ar_dbg(">>", ar);

  ar->offset = 0;
  ar->err = NULL;

  ar_dbg("<<", ar);
  return true;
}

/* this is to enable cross platform tests that assume an alignment */
#ifndef SA_DEFAULT_ALIGNMENT
#define SA_DEFAULT_ALIGNMENT sizeof(max_align_t)
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
  /* this guard is so that we don't return an alignment value > SA_DEFAULT_ALIGNMENT */
  if (sz < SA_DEFAULT_ALIGNMENT) {
    /* the branches below return "natural alignment" for values belows SA_DEFAULT_ALIGNMENT */
    /* the code below obviously assumes SA_DEFAULT_ALIGNMENT to be a max of 32 */
    if (sz <= 1) {
      return sz;
    }
    if (sz <= 2) {
      return 2;
    }
    if (sz <= 4) {
      return 4;
    }
    if (sz <= 8) {
      return 8;
    }
    if (sz <= 16) {
      return 16;
    }
  }
  return SA_DEFAULT_ALIGNMENT;
}

/*
 * DESCRIPTION
 *
 * This allocates size bytes in the arena and returns a pointer to the beginning of
 * the allocated memory. It is an aligned allocator. It uses natural alignment for
 * size values below sizeof(max_align_t). Greater than that, it aligns to sizeof(max_align_t).
 *
 * RETURN VALUES
 *
 * If arena is "valid" and size is > zero it either returns a pointer to the beginning
 * of the allocated memory or NULL if out of memory.
 * If either arena is "invalid" or size is <= 0, then it sets the "err" property of
 * the arena with the error message and returns NULL.
 */
void *arena_alloc(arena_t *const ar, const size_t sz)
{
  ar_dbg(">>", ar);
  dbg(">> requested size %zu", sz);

  /*
   * Strict validity check as we don't want to allocate in invalid arenas to prevent
   * data loss for the user.
   * Although, We don't check for ar->err, as, for e.g. a previous allocation attempt
   * might have failed due to ARENA_ENOMEM but the currently requested allocation
   * _might_ fit in the remaining arena memory. We want to allow that.
   *
   * Also, as mentioned in the struct definition of arena_t,  ar->err is more for
   * signaling error to the caller rather than using it for checks within functions
   * in this lib. Functions here should do their own validations on the arena before
   * proceeding or signal an error of their own. They should always clobber ar->err
   * with their own reason for failure.
   */
  if (sz <= 0 || ar->arena == NULL || ar->size <= 0 || ar->offset < 0 || ar->offset > ar->size) {
    ar->err = arena_get_err_msg_(ARENA_EINVAL);

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
      dbg("++ padding %zu", padding);
    }
    /* not using ptrdiff_t as ptr is guaranteed to be greater than ar->arena due to checks and ptr assignment above */
    size_t ptr_offset = ptr - (uintptr_t)ar->arena;
    size_t remaining = ar->size - ptr_offset;
    dbg("++ remaining %zu", remaining);

    /* non-resizeable arena hence we return NULL to signal error */
    if (sz > remaining) {
      ar->err = arena_get_err_msg_(ARENA_ENOMEM);

      ar_dbg("<<", ar);
      return NULL;
    }

    dbg("++ padded ptr %p", (void *)ptr);

    /* we need to point to address after the one we are returning in ptr */
    ar->offset = ptr_offset + sz;
  }

  /* clear errors as the allocation was successful */
  ar->err = NULL;

  ar_dbg("<<", ar);
  dbg("<< allocated ptr %p", (void *)ptr);
  return (void *)ptr;
}

/*
 * DESCRIPTION
 *
 * This function allocates memory for "count" objects, each of size bytes. It then zero-fills
 * the memory region and returns a pointer to the beginning of the allocated memory.
 *
 * RETURN VALUES
 *
 * If arena is "valid" and (count * size) is > zero it either returns a pointer to the beginning
 * of the allocated memory or NULL if out of memory. If either arena is "invalid" or size is <= 0,
 * then it sets the "err" property of the arena with the error message and returns NULL.
 */
void *arena_calloc(arena_t *const ar, const size_t count, const size_t sz)
{
  dbg(">> count %zu \t| size %zu", count, sz);
  size_t total_size = count * sz;
  void *ptr = arena_alloc(ar, total_size);
  /* not zeroing memory before returning as unix, linux and macos return zero-filled memory on MAP_ANONYMOUS */
  /* and this function is guarded for those OS-es */

  dbg("<< allocated ptr %p", ptr);
  return ptr;
}

#elif defined(_WIN32) || defined(_WIN64)
/* No support for windows yet */
#else
/* Unsupported OSes */
#endif

#endif // ZDX_SIMPLE_ARENA_IMPLEMENTATION
