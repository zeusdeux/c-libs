#ifndef ZDX_MEMORY_H_
#define ZDX_MEMORY_H_

#ifndef MEM_API
#define MEM_API
#endif // MEM_API

#include <stddef.h>

/**
 * To understand function pointers -
 *
 * extern void (*signal(int, void(*)(int)))(int); <-- the outermost `void` and outermost `(int)`
 *                                                    come together to make the return type of
 *                                                    signal be a function void (*)(int) aka
 *                                                    SignalHandler below.
 * ^^^^^^ is the same as below
 * typedef void (*SignalHandler)(int signum);
 * extern SignalHandler signal(int signum, SignalHandler handler);
 */

typedef void* (*mem_malloc_fn_t)(void *ctx, const size_t sz);
typedef void* (*mem_calloc_fn_t)(void *ctx, const size_t count, const size_t sz);
typedef void* (*mem_realloc_fn_t)(void *ctx, void *ptr, const size_t old_sz, const size_t new_sz);
typedef void  (*mem_free_fn_t)(void *ctx, void *ptr);
typedef void  (*mem_empty_fn_t)(void *ctx);
// TODO(mudit): should it deinit the allocator and thus null out ctx
// after deinit(ctx)? or should it only deinit the ctx and leave *ctx
// as a valid pointer?
typedef void  (*mem_deinit_fn_t)(void *ctx);

typedef struct {
  void *ctx;
  mem_malloc_fn_t alloc;
  mem_calloc_fn_t calloc;
  mem_realloc_fn_t realloc;
  mem_free_fn_t free;
  mem_empty_fn_t empty;
  mem_deinit_fn_t deinit;
} mem_allocator_t;

MEM_API mem_allocator_t mem_gpa_init(const char name[const static 1]);

// ----------------------------------------------------------------------------------------------------------------

#ifdef ZDX_MEMORY_IMPLEMENTATION_AUTO
#  define ZDX_STRING_VIEW_IMPLEMENTATION
#  define ZDX_MEMORY_IMPLEMENTATION
#endif // ZDX_MEMORY_IMPLEMENTATION_AUTO

#ifdef ZDX_MEMORY_IMPLEMENTATION

#include <stdlib.h>
#include "zdx_string_view.h"
#include "zdx_util.h"

#ifndef MEM_ASSERT
#define MEM_ASSERT assertm
#endif // MEM_ASSERT

typedef struct {
  sv_t name;
} gpa_context_t;

#define MEM_ASSERT_NONNULL(ctx) MEM_ASSERT(ctx != NULL, "Expected: context to be non-NULL, Received: %p", ctx)

static void *gpa_malloc(void *ctx, const size_t sz)
{
  (void) ctx;
  MEM_ASSERT_NONNULL(ctx);
  dbg(">> [allocator "SV_FMT"]: size = %zu", sv_fmt_args(((gpa_context_t *)ctx)->name), sz);

  void *ptr = malloc(sz);

  dbg("<< [allocator "SV_FMT"]: %p", sv_fmt_args(((gpa_context_t *)ctx)->name), ptr);
  return ptr;
}

static void *gpa_calloc(void *ctx, const size_t count, const size_t sz)
{
  (void) ctx;
  MEM_ASSERT_NONNULL(ctx);
  dbg(">> [allocator "SV_FMT"]: count = %zu, size = %zu",
      sv_fmt_args(((gpa_context_t *)ctx)->name), count, sz);

  void *ptr = calloc(count, sz);

  dbg("<< [allocator "SV_FMT"]: %p", sv_fmt_args(((gpa_context_t *)ctx)->name), ptr);
  return ptr;
}

static void *gpa_realloc(void *ctx, void *ptr, const size_t old_sz, const size_t new_sz)
{
  (void) ctx;
  (void) old_sz;
  MEM_ASSERT_NONNULL(ctx);
  dbg(">> [allocator "SV_FMT"]: ptr = %p, old size = %zu, new size = %zu",
      sv_fmt_args(((gpa_context_t *)ctx)->name), ptr, old_sz, new_sz);

  void *new_ptr = realloc(ptr, new_sz);

  dbg("<< [allocator "SV_FMT"]: realloced ptr = %p", sv_fmt_args(((gpa_context_t *)ctx)->name), new_ptr);
  return new_ptr;
}

static void gpa_free(void *ctx, void *ptr)
{
  (void) ctx;
  MEM_ASSERT_NONNULL(ctx);
  dbg(">> [allocator "SV_FMT"]: ptr = %p", sv_fmt_args(((gpa_context_t *)ctx)->name), ptr);

  dbg("<< [allocator "SV_FMT"]", sv_fmt_args(((gpa_context_t *)ctx)->name));
  free(ptr);
}

static void gpa_empty(void *ctx)
{
  (void) ctx;
  return;
}

static void gpa_deinit(void *ctx)
{
  MEM_ASSERT_NONNULL(ctx);
  dbg(">> [allocator "SV_FMT"]", sv_fmt_args(((gpa_context_t *)ctx)->name));

  gpa_context_t *gpa_ctx = ctx;
  sv_t name = gpa_ctx->name;

  gpa_free(ctx, ctx); // ctx is both the ctx and the pointer we want freed

  // reset the name string view. If it holds a dynamically allocated pointer,
  // it's whoever allocated it that needs to free it as string view never
  // allocates and is only a readonly view on the buffer pointer it holds.
  gpa_ctx->name = (sv_t){0};

  dbg("<< [allocator "SV_FMT"]", sv_fmt_args(name));
  return;
}

/**
 * GPA = General Purpose Allocator
 *   This function initializes a malloc and friends based general purpose allocator
 *
 * Example:
 *   mem_allocator_t gpa = mem_gpa_init("test");
 *   int *a = gpa.alloc(gpa.ctx, sizeof(*a) * 10);
 *   ...
 *   ...
 *   gpa.free(gpa.ctx, a);
 *   ...
 *   ...
 *   gpa.deinit(gpa.ctx);
 */
MEM_API mem_allocator_t mem_gpa_init(const char name[const static 1])
{
  MEM_ASSERT_NONNULL((void *)name);
  dbg(">> name = %s", name);

  const gpa_context_t gla_default_ctx = { .name = {"Default", 7} };

  gpa_context_t *ctx = gpa_malloc((void *)&gla_default_ctx, sizeof(*ctx));
  ctx->name = sv_from_cstr(name);

  dbg("<< [allocator "SV_FMT"]", sv_fmt_args(ctx->name));
  return (mem_allocator_t){
    .ctx = ctx,
    .alloc = gpa_malloc,
    .calloc = gpa_calloc,
    .realloc = gpa_realloc,
    .free = gpa_free,
    .empty = gpa_empty,
    .deinit = gpa_deinit,
  };
}

#endif // ZDX_MEMORY_IMPLEMENTATION
#endif // ZDX_MEMORY_H_
