#ifndef ZDX_MEMORY_H_
#define ZDX_MEMORY_H_

#ifndef MEM_API
#define MEM_API
#endif // MEM_API

#ifdef ZDX_MEMORY_IMPLEMENTATION_AUTO
#  define ZDX_STRING_VIEW_IMPLEMENTATION
#  define ZDX_MEMORY_IMPLEMENTATION
#endif // ZDX_MEMORY_IMPLEMENTATION_AUTO

#include <stddef.h>
#include "zdx_string_view.h"
#include "zdx_util.h"

typedef struct mem_allocator_t mem_allocator_t;

/**
 * To understand function pointers -
 *
 * extern void (*signal(int, void(*)(int)))(int); <-- the outermost `void` and outermost `(int)`
 *                                                    come together to make the return type of
 *                                                    signal be a function void (*)(int) aka
 *                                                    SignalHandler below.
 * ^^^^^^ is the same as below:
 * typedef void (*SignalHandler)(int signum);
 * extern SignalHandler signal(int signum, SignalHandler handler);
 */

typedef void* (*mem_malloc_fn_t)(const mem_allocator_t *const al, const size_t sz);
typedef void* (*mem_calloc_fn_t)(const mem_allocator_t *const al, const size_t count, const size_t sz);
typedef void* (*mem_realloc_fn_t)(const mem_allocator_t *const al, void *ptr, const size_t old_sz, const size_t new_sz);
typedef void  (*mem_free_fn_t)(const mem_allocator_t *const al, void *ptr);
typedef void  (*mem_empty_fn_t)(const mem_allocator_t *const al);
// TODO(mudit): should it deinit the allocator and thus null out ctx
// after deinit(ctx)? or should it only deinit the ctx and leave *ctx
// as a valid pointer?
typedef void  (*mem_deinit_fn_t)(mem_allocator_t *const al);

typedef struct mem_allocator_t {
  void *ctx;
  sv_t name;
  mem_malloc_fn_t alloc;
  mem_calloc_fn_t calloc;
  mem_realloc_fn_t realloc;
  mem_free_fn_t free;
  mem_empty_fn_t empty;
  mem_deinit_fn_t deinit;
} mem_allocator_t;

MEM_API mem_allocator_t mem_gpa_init(const char name_cstr[const static 1]);

// ----------------------------------------------------------------------------------------------------------------


#ifdef ZDX_MEMORY_IMPLEMENTATION

#include <stdlib.h>

#ifndef MEM_ASSERT
#define MEM_ASSERT assertm
#endif // MEM_ASSERT

#define MEM_ASSERT_NONNULL(val) MEM_ASSERT((val) != NULL, "Expected: context to be non-NULL, Received: %p", (void *)(val))

static void *gpa_malloc(const mem_allocator_t *const al, const size_t sz)
{
  (void) al;
  MEM_ASSERT_NONNULL(al);

  dbg(">> [allocator "SV_FMT"]: size = %zu", sv_fmt_args(al->name), sz);

  void *ptr = malloc(sz);

  dbg("<< [allocator "SV_FMT"]: %p", sv_fmt_args(al->name), ptr);
  return ptr;
}

static void *gpa_calloc(const mem_allocator_t *const al, const size_t count, const size_t sz)
{
  (void) al;
  MEM_ASSERT_NONNULL(al);

  dbg(">> [allocator "SV_FMT"]: count = %zu, size = %zu",
      sv_fmt_args(al->name), count, sz);

  void *ptr = calloc(count, sz);

  dbg("<< [allocator "SV_FMT"]: %p", sv_fmt_args(al->name), ptr);
  return ptr;
}

static void *gpa_realloc(const mem_allocator_t *const al, void *ptr, const size_t old_sz, const size_t new_sz)
{
  (void) al;
  MEM_ASSERT_NONNULL(al);

  (void) old_sz;

  dbg(">> [allocator "SV_FMT"]: ptr = %p, old size = %zu, new size = %zu",
      sv_fmt_args(al->name), ptr, old_sz, new_sz);

  void *new_ptr = realloc(ptr, new_sz);

  dbg("<< [allocator "SV_FMT"]: realloced ptr = %p", sv_fmt_args(al->name), ptr);
  return new_ptr;
}

static void gpa_free(const mem_allocator_t *const al, void *ptr)
{
  (void) al;
  MEM_ASSERT_NONNULL(al);

  dbg(">> [allocator "SV_FMT"]: ptr = %p", sv_fmt_args(al->name), ptr);

  dbg("<< [allocator "SV_FMT"]", sv_fmt_args(al->name));
  free(ptr);
}

static void gpa_empty(const mem_allocator_t *const al)
{
  (void) al;
  return;
}

static void gpa_deinit(mem_allocator_t *const al)
{
  MEM_ASSERT_NONNULL(al);

  dbg(">> [allocator "SV_FMT"]", sv_fmt_args(al->name));

  sv_t name = al->name;
  (void) name;

  al->name = (sv_t){0};
  free(al->ctx);

  // reset the name string view. If it holds a dynamically allocated pointer,
  // it's whoever allocated it that needs to free it as string view never
  // allocates and is only a readonly view on the buffer pointer it holds.

  dbg("<< [allocator "SV_FMT"] Destroyed!", sv_fmt_args(name));
  return;
}

/**
 * GPA = General Purpose Allocator
 *   This function initializes a malloc and friends based general purpose allocator
 *
 * Example:
 *   mem_allocator_t gpa = mem_gpa_init("test");
 *   int *a = gpa.alloc(&gpa, sizeof(*a) * 10);
 *   ...
 *   ...
 *   gpa.free(&gpa, a);
 *   ...
 *   ...
 *   gpa.deinit(&gpa);
 */
MEM_API mem_allocator_t mem_gpa_init(const char name_cstr[const static 1])
{
  MEM_ASSERT_NONNULL((void *)name_cstr);
  dbg(">> name = %s", name_cstr);

  mem_allocator_t allocator = {
    .ctx = NULL,
    .name = sv_from_cstr(name_cstr),
    .alloc = gpa_malloc,
    .calloc = gpa_calloc,
    .realloc = gpa_realloc,
    .free = gpa_free,
    .empty = gpa_empty,
    .deinit = gpa_deinit,
  };

  dbg("<< [allocator "SV_FMT"]", sv_fmt_args(allocator.name));
  return allocator;
}

#endif // ZDX_MEMORY_IMPLEMENTATION
#endif // ZDX_MEMORY_H_
