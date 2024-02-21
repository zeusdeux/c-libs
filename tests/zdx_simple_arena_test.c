#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include "../zdx_util.h"
#define SA_DEFAULT_ALIGNMENT 8 // to make sure tests work as expected across platforms
#define ZDX_SIMPLE_ARENA_IMPLEMENTATION
#include "../zdx_simple_arena.h"

void test_arena_alloc(arena_t *const arena, const size_t sz, const size_t expected_offset, const size_t expected_alignment);

int main(void)
{
  size_t requested_arena_size = 4098;

  /* arena_create */
  {
    {
    arena_t arena = arena_create(requested_arena_size);
    assertm(!arena.err, "Expected: valid arena to be created, Received: %s -> %s", arena.err, strerror(errno));
    assertm(arena.arena != NULL, "Expected: non-NULL arena addr, Received: %p", arena.arena);

    size_t expected_arena_size = arena_round_up_to_page_size_(requested_arena_size);
    assertm(arena.size == expected_arena_size, "Expected: %ld, Received: %zu", expected_arena_size, arena.size);
    assertm(arena.offset == 0, "Expected: 0, Received: %zu", arena.offset);

    assertm(arena_free(&arena) && !arena.err, "Expected: arena free to work, Received: %s -> %s", arena.err,  strerror(errno));

    log(L_INFO, "[ARENA CREATE HAPPY PATH TESTS] OK!");
    }

    {
      // TODO: add error path tests for arena_create by setting size to negative somehow
      // as that'll make mmap fail
    }
  }

  /* arena_free */
  {
    {
      arena_t arena = arena_create(requested_arena_size);
      assertm(!arena.err, "Expected: valid arena to be created, Received: %s -> %s", arena.err, strerror(errno));

      assertm(arena_free(&arena) && arena.err == NULL, "Expected: arena free to work, Received: %s -> %s", arena.err,  strerror(errno));
      assertm(arena.arena == NULL, "Expected: non-NULL arena addr, Received: %p", arena.arena);
      assertm(arena.size == 0, "Expected: 0, Received: %zu", arena.size);
      assertm(arena.offset == 0, "Expected: 0, Received: %zu", arena.offset);

      arena = arena_create(requested_arena_size);
      assertm(!arena.err, "Expected: valid arena to be created, Received: %s -> %s", arena.err, strerror(errno));
      /* arena.err should have no impact on running arena_free */
      arena.err = "SOME ERROR";
      assertm(arena_free(&arena) && arena.err == NULL, "Expected: arena free to work, Received: %s -> %s", arena.err,  strerror(errno));
      assertm(arena.arena == NULL, "Expected: non-NULL arena addr, Received: %p", arena.arena);
      assertm(arena.size == 0, "Expected: 0, Received: %zu", arena.size);
      assertm(arena.offset == 0, "Expected: 0, Received: %zu", arena.offset);

      arena = arena_create(requested_arena_size);
      assertm(!arena.err, "Expected: valid arena to be created, Received: %s -> %s", arena.err, strerror(errno));
      /* invalid arena.offset should have no impact on running arena_free */
      arena.offset = arena.size + 100;
      assertm(arena_free(&arena) && arena.err == NULL, "Expected: arena free to work, Received: %s -> %s", arena.err,  strerror(errno));
      assertm(arena.arena == NULL, "Expected: non-NULL arena addr, Received: %p", arena.arena);
      assertm(arena.size == 0, "Expected: 0, Received: %zu", arena.size);
      assertm(arena.offset == 0, "Expected: 0, Received: %zu", arena.offset);

      log(L_INFO, "[ARENA FREE HAPPY PATH TESTS] OK!");
    }

    {
      // TODO: add error path tests for arena_free by setting ar->size to
      // negative somehow for example as that'll make munmap fail
    }
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


      /* arena.err being set should have no impact on allocation */
      const char *err_msg = "SOME ERROR";
      arena.err = err_msg;
      test_arena_alloc(&arena, 10, 50, 8);
      /* reset arena to proper state */
      arena.err = NULL;


      /* struct allocation */
      typedef struct {
        char a;
        char b;
      } two_chars;
      two_chars *t = arena_alloc(&arena, sizeof(two_chars));
      assertm(!arena.err, "Expected: arena_alloc to succeed, Received: %s", arena.err);

      t->a = 'a';
      t->b = 'b';
      assertm(t->a == 'a', "Expected: 'a', Received: %c", t->a);
      assertm(t->b == 'b', "Expected: 'b', Received: %c", t->b);

      /* test that allocation fails when there's no enough memory for one allocation but passes for another */
      size_t arena_offset = arena.offset;
      size_t arena_offset_new = arena.size - 1; // pretend only 1 free byte remaining in arena
      arena.offset = arena_offset_new;
      char *ch = arena_alloc(&arena, 2);
      (void) ch;
      assertm(arena.err, "Expected: arena_alloc to fail, Received: %s", arena.err);
      assertm(arena.offset == arena_offset_new, "Expected: %zu (unchanged offset), Received: %zu", arena_offset_new, arena.offset);
      ch = arena_alloc(&arena, 1);
      assertm(arena.err == NULL, "Expected: arena_alloc to succeed, Received: %s", arena.err);
      assertm(arena.offset == arena.size, "Expected: %zu, Received: %zu", arena.size, arena.offset);
      arena.offset = arena_offset;

      assertm(arena_free(&arena) && !arena.err,
              "Expected: arena free to work, Received: %s -> %s", arena.err,  strerror(errno));

      log(L_INFO, "[ARENA ALLOC HAPPY PATH TESTS] OK!");
    }

    {
      arena_t arena = arena_create(requested_arena_size);
      assertm(!arena.err, "Expected: valid arena to be created, Received: %s -> %s", arena.err, strerror(errno));

      /* 0 byte allocation */
      int *k = arena_alloc(&arena, 0);
      assertm(arena.err, "Expected: arena_alloc to fail for zero byte allocations, Received: %s", arena.err);
      assertm(k == NULL, "Expected: NULL, Received: %p", (void *)k);
      assertm(arena.offset == 0, "Expected: arena offset to not change, Received: %zu", arena.offset);

      /* test arena size less than 0 */
      /* arena.size = -1; // this should trigger compiler error due to #pragma GCC diagnostic error "-Wsign-conversion" */

      /* test invalid arena size of 0 */
      size_t arena_sz = arena.size;
      arena.size = 0; // trigger error in arena_alloc
      int *i = arena_alloc(&arena, 4);
      (void) i;
      assertm(arena.err, "Expected: Arena to show error: \"Error: %s\", Received: valid arena", arena.err);
      /* reset arena to proper state */
      arena.size = arena_sz;
      arena.err = NULL;

      /* test arena offset less that 0 */
      /* arena.offset = -1; // this should trigger compiler error due to #pragma GCC diagnostic error "-Wsign-conversion" */

      /* test arena offset beyond arena size */
      arena.offset = arena.size + 1; // trigger error in arena_alloc
      i = arena_alloc(&arena, 10);
      assertm(arena.err, "Expected: Arena to show error: \"Error: %s\", Received: valid arena", arena.err);
      /* reset arena to proper state */
      arena.offset = 0;
      arena.err = NULL;

      /* test invalid arena base address (NULL) */
      void *arena_addr = arena.arena;
      arena.arena = NULL;
      i = arena_alloc(&arena, 20);
      assertm(arena.err, "Expected: Arena to show error: \"Error: %s\", Received: valid arena", arena.err);
      /* reset arena to proper state */
      arena.arena = arena_addr;
      arena.err = NULL;

      /* test arena out of memory */
      i = arena_alloc(&arena, 40); // bump the offset a bit
      arena_sz = arena.size;
      arena.size = 42;
      i = arena_alloc(&arena, 4);
      assertm(arena.err, "Expected: Arena to show error: \"Error: %s\", Received: valid arena", arena.err);
      /* reset arena to proper state */
      arena.size = arena_sz;
      arena.err = NULL;

      assertm(arena_free(&arena) && !arena.err,
              "Expected: arena free to work, Received: %s -> %s", arena.err,  strerror(errno));

      log(L_INFO, "[ARENA ALLOC ERROR PATH TESTS] OK!");
    }
  }

  /* arena_reset */
  {
    {
      arena_t arena = arena_create(requested_arena_size);
      void *arena_base_ptr = arena.arena;
      assertm(!arena.err, "Expected: valid arena to be created, Received: %s -> %s", arena.err, strerror(errno));

      int *i = arena_alloc(&arena, sizeof(int));
      (void) i;
      assertm(!arena.err, "Expected: Arena to show error: \"Error: %s\", Received: valid arena", arena.err);
      assertm(arena.offset == 4, "Expected: 4, Received: %zu", arena.offset);

      assertm(arena_reset(&arena) && !arena.err,
              "Expected: arena_reset to work, Received: %s -> %s", arena.err,  strerror(errno));
      assertm(arena.offset == 0, "Expected: arena offset to reset to 0, Received: %zu", arena.offset);
      assertm(arena.arena == arena_base_ptr, "Expected: arena offset to remain unchanged from %p, Received: %p", arena_base_ptr, arena.arena);

      /* arena.err should have no impact on arena_reset */
      const char *err_msg = "SOME ERROR";
      arena.err = err_msg;
      assertm(arena.offset == 0, "Expected: arena offset to be 0, Received: %zu", arena.offset);
      assertm(arena_reset(&arena), "Expected: arena reset to succeed, Received: false (%s)", arena.err);
      assertm(arena.err == NULL, "Expected: arena error to be reset to NULL, Received: %s", arena.err);

      // TODO: add a test where arena_alloc fails and then we arena_reset and alloc again and that passes

      assertm(arena_free(&arena) && !arena.err,
              "Expected: arena free to work, Received: %s -> %s", arena.err,  strerror(errno));

      log(L_INFO, "[ARENA RESET HAPPY PATH TESTS] OK!");
    }

    {
      // arena_reset has no error paths as it always returns true
    }
  }

  /* arena_calloc */
  {
    {
      arena_t arena = arena_create(requested_arena_size);
      assertm(!arena.err, "Expected: valid arena to be created, Received: %s -> %s", arena.err, strerror(errno));

      typedef struct {
        int i;
        double d;
        struct {
          size_t capacity;
          size_t len;
          char *str;
        } sb;
      } my_struct;
      my_struct *m_arr = arena_calloc(&arena, 10, sizeof(my_struct));
      size_t zero_count = 0;
      size_t calloced_bytes = sizeof(my_struct) * 10;

      for (size_t i = 0; i < calloced_bytes; i++) {
        /* same as *((uint8_t *)m_arr + i) */
        uint8_t val = ((uint8_t *)m_arr)[i];
        val == 0 && ++zero_count;
      }

      assertm(zero_count == calloced_bytes,
              "Expected: %zu bytes to be zero filled, Received: %zu bytes were zero filled", zero_count, calloced_bytes);

      assertm(arena_free(&arena) && !arena.err,
              "Expected: arena free to work, Received: %s -> %s", arena.err,  strerror(errno));


      log(L_INFO, "[ARENA CALLOC HAPPY PATH TESTS] OK!");
    }

    {
      // All arena_calloc error path tests are the ones checked by arena_alloc as arena_calloc
      // calls arena_alloc internally and offloads all error checking to arena_alloc
      log(L_INFO, "[ARENA CALLOC ERROR PATH TESTS] OK!");
    }
  }

  {
    // TODO: tests for arena_realloc
  }

  log(L_INFO, "<zdx_simple_arena_test> All ok!\n");
  return 0;
}

void test_arena_alloc(arena_t *const arena, const size_t sz, const size_t expected_offset, const size_t expected_alignment)
{
  /* log(L_INFO, "[ARENA ALLOC TEST] size %zu, exp offset %zu, exp align %zu", sz, expected_offset, expected_alignment); */
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
