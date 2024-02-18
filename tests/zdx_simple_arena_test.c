#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include "../zdx_util.h"
#define ZDX_SIMPLE_ARENA_IMPLEMENTATION
#include "../zdx_simple_arena.h"

void test_arena_alloc(arena_t *const arena, const size_t sz, const size_t expected_offset, const size_t expected_alignment);

int main(void)
{
  size_t requested_arena_size = 4098;

  /* arena_create */
  {
    arena_t arena = arena_create(requested_arena_size);
    assertm(!arena.err, "Expected: valid arena to be created, Received: %s -> %s", arena.err, strerror(errno));
    assertm(arena.arena != NULL, "Expected: non-NULL arena addr, Received: %p", arena.arena);

    size_t expected_arena_size = arena_round_up_to_page_size_(requested_arena_size);
    assertm(arena.size == expected_arena_size, "Expected: %ld, Received: %zu", expected_arena_size, arena.size);
    assertm(arena.offset == 0, "Expected: 0, Received: %zu", arena.offset);

    assertm(arena_free(&arena) && !arena.err, "Expected: arena free to work, Received: %s -> %s", arena.err,  strerror(errno));

    log(L_INFO, "[ARENA CREATE TESTS] OK!");
  }

  /* arena_free */
  {
    arena_t arena = arena_create(requested_arena_size);
    assertm(!arena.err, "Expected: valid arena to be created, Received: %s -> %s", arena.err, strerror(errno));

    assertm(arena_free(&arena) && !arena.err, "Expected: arena free to work, Received: %s -> %s", arena.err,  strerror(errno));
    assertm(arena.arena == NULL, "Expected: non-NULL arena addr, Received: %p", arena.arena);
    assertm(arena.size == 0, "Expected: 0, Received: %zu", arena.size);
    assertm(arena.offset == 0, "Expected: 0, Received: %zu", arena.offset);

    log(L_INFO, "[ARENA FREE TESTS] OK!");
  }

  /* arena_alloc */
  {
    {
      arena_t arena = arena_create(requested_arena_size);
      assertm(!arena.err, "Expected: valid arena to be created, Received: %s -> %s", arena.err, strerror(errno));

      /* 1 byte allocation */
      test_arena_alloc(&arena, 1, 1, 1);
      /* 2 bytes allocation */
      test_arena_alloc(&arena, 2, 4, 2);
      /* 3 bytes allocation */
      test_arena_alloc(&arena, 3, 7, 4);
      /* 5 bytes allocation */
      test_arena_alloc(&arena, 5, 13, 8); // (3 from above get's padded to 8 to be divisible by alignment of 8) + 5 = 13
      /* 4 bytes allocation */
      test_arena_alloc(&arena, 4, 20, 4); // (13 from above get's padded to 16 to be divisible by alignment of 4) + 4 = 20
      /* 11 bytes allocation */
      test_arena_alloc(&arena, 11, 35, 8); // (20 from above get's padded to 24 to be divisible by alignment of 8) + 5 = 29

      /* struct allocation */
      typedef struct {
        char a;
        char b;
      } two_chars;
      two_chars *t = arena_alloc(&arena, sizeof(two_chars));
      t->a = 'a';
      t->b = 'b';
      assertm(t->a == 'a', "Expected: 'a', Received: %c", t->a);
      assertm(t->b == 'b', "Expected: 'b', Received: %c", t->b);

      assertm(arena_free(&arena) && !arena.err,
              "Expected: arena free to work, Received: %s -> %s", arena.err,  strerror(errno));

      log(L_INFO, "[ARENA ALLOC HAPPY PATH TESTS] OK!");
    }
    {
      arena_t arena = arena_create(requested_arena_size);
      assertm(!arena.err, "Expected: valid arena to be created, Received: %s -> %s", arena.err, strerror(errno));

      /* arena.size = -1; // this should trigger compiler error */
      size_t arena_sz = arena.size;
      arena.size = 0; // trigger error in arena_alloc
      int *i = arena_alloc(&arena, 4);
      (void) i;
      assertm(arena.err, "Expected: Arena to show error: %s, Received: valid arena", arena.err);
      /* reset arena to proper state */
      arena.size = arena_sz;
      arena.err = NULL;

      assertm(arena_free(&arena) && !arena.err,
              "Expected: arena free to work, Received: %s -> %s", arena.err,  strerror(errno));

      log(L_INFO, "[ARENA ALLOC ERROR PATHS TESTS] OK!");
    }
  }

  log(L_INFO, "<zdx_gap_buffer_test> All ok!\n");
  return 0;
}

void test_arena_alloc(arena_t *const arena, const size_t sz, const size_t expected_offset, const size_t expected_alignment)
{
  log(L_INFO, "[ARENA ALLOC TEST] size %zu, exp offset %zu, exp align %zu", sz, expected_offset, expected_alignment);
  assertm(sz > 0, "Expected: test_arena_alloc called with size > 0, Received: %zu", sz);

  int8_t *n_bytes = arena_alloc(arena, sz);
  assertm(!arena->err, "Expected: arena_alloc to succeed, Received: %s", arena->err);
  assertm(n_bytes != NULL, "Expected: a non-NULL allocation, Received: %p", (void *)n_bytes);
  assertm(arena->offset == expected_offset, "Expected: %zu, Received: %zu", expected_offset, arena->offset);

  uint64_t n_bytes_alignment = (uintptr_t)n_bytes % expected_alignment;
  assertm(n_bytes_alignment == 0, "Expected: %p aligned to 4, Received: %llu",(void *)n_bytes, n_bytes_alignment);

  if (sz < 2) {
    char c = 'a';
    n_bytes[0] = c;
    assertm(n_bytes[0] == c, "Expected: %c, Received: %c", c, (char)n_bytes[0]);
  }
  if (sz < 3) {
    n_bytes[1] = 20;
    assertm(n_bytes[1] == 20, "Expected: 20, Received: %d", n_bytes[1]);
  }
  n_bytes[2] = -100;
  assertm(n_bytes[2] == -100, "Expected: -100, Received: %d", n_bytes[2]);

  /* the below should seg fault once once allocator protects padded memory correct */
  /* n_bytes[sz] = 50; */
  /* assertm(n_bytes[sz], "Nope"); // this should segfault */
}