#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <inttypes.h>

#include "../zdx_test_utils.h"

#define SA_DEFAULT_ALIGNMENT 8 // to make sure tests work as expected across platforms
#define ZDX_SIMPLE_ARENA_IMPLEMENTATION
#include "../zdx_simple_arena.h"

static void test_arena_alloc(arena_t *const arena, const size_t sz, const size_t expected_offset, const size_t expected_alignment);

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

      assertm(arena_free(&arena) && !arena.err,
              "Expected: arena free to work, Received: %s -> %s", arena.err,  strerror(errno));

      testlog(L_INFO, "[ARENA CREATE HAPPY PATH TESTS] OK!");
    }

    {
      // make mmap fail by requesting 0 bytes
      arena_t arena = arena_create(0);
      assertm(arena.err, "Expected: arena creation to fail with '%s -> %s', Received: valid arena", arena.err, strerror(errno));
      assertm(arena.arena == NULL, "Expected: NULL as arena base addr, Received: %p", arena.arena);
      assertm(arena.size == 0, "Expected: %d, Received: %zu", 0, arena.size);
      assertm(arena.offset == 0, "Expected: 0, Received: %zu", arena.offset);

      // arena.size being 0 will cause munmap to fail as well
      assertm(!arena_free(&arena) && arena.err,
              "Expected: free-ing an unallocated arena should fail with '%s -> %s', Received: arena_free worked", arena.err,  strerror(errno));

      testlog(L_INFO, "[ARENA CREATE ERROR PATH TESTS] OK!");
    }

#ifdef DEBUG
    {
      /* DEBUG being defined means it's a debug build, therefore arena_create should memset arena to 0xcd */
      arena_t arena = arena_create(requested_arena_size);
      assertm(!arena.err, "Expected: valid arena to be created, Received: %s -> %s", arena.err, strerror(errno));
      assertm(arena.arena != NULL, "Expected: non-NULL arena addr, Received: %p", arena.arena);

      size_t expected_arena_size = arena_round_up_to_page_size_(requested_arena_size);
      assertm(arena.size == expected_arena_size, "Expected: %ld, Received: %zu", expected_arena_size, arena.size);
      assertm(arena.offset == 0, "Expected: 0, Received: %zu", arena.offset);

      for (size_t i = 0; i < arena.size; i++) {
        uint8_t val = ((uint8_t *)arena.arena)[i];
        assertm(val == SA_DEBUG_BYTE, "Expected: Byte %zu to be %#x, Received: %u", i, SA_DEBUG_BYTE, val);
      }

      assertm(arena_free(&arena) && !arena.err,
              "Expected: arena free to work, Received: %s -> %s", arena.err,  strerror(errno));

      testlog(L_INFO, "[ARENA CREATE DEBUG PATH TESTS] OK!");
    }
#endif
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

      testlog(L_INFO, "[ARENA FREE HAPPY PATH TESTS] OK!");
    }

    {
      // make mmap fail by requesting 0 bytes
      arena_t arena = arena_create(0);
      // arena.size being 0 will cause munmap to fail as well
      assertm(!arena_free(&arena) && arena.err,
              "Expected: free-ing an unallocated arena should fail with '%s -> %s', Received: arena_free worked", arena.err,  strerror(errno));
      assertm(arena.err, "Expected: arena creation to fail with '%s -> %s', Received: valid arena", arena.err, strerror(errno));
      assertm(arena.arena == NULL, "Expected: NULL as arena base addr, Received: %p", arena.arena);
      assertm(arena.size == 0, "Expected: %d, Received: %zu", 0, arena.size);
      assertm(arena.offset == 0, "Expected: 0, Received: %zu", arena.offset);

      testlog(L_INFO, "[ARENA FREE ERROR PATH TESTS] OK!");
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

      testlog(L_INFO, "[ARENA ALLOC HAPPY PATH TESTS] OK!");
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

      testlog(L_INFO, "[ARENA ALLOC ERROR PATH TESTS] OK!");
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
      assertm(arena_reset(&arena), "Expected: arena reset to succeed, Received: false (%s)", arena.err);
      assertm(arena.offset == 0, "Expected: arena offset to be 0, Received: %zu", arena.offset);
      assertm(arena.err == NULL, "Expected: arena error to be reset to NULL, Received: %s", arena.err);

      /* a test where arena_alloc fails with ARENA_ENOMEM and then we arena_reset and alloc again and that passes */
      arena_base_ptr = arena.arena;
      size_t arena_size = arena.size;
      size_t new_offset = arena.size - 1; // 1 byte remaining in arena
      arena.offset = new_offset;
      char *c = arena_alloc(&arena, 2); // request 2 bytes of allocation
      (void) c;
      assertm(arena.err,
              "Expected: Arena to show error: \"Error: %s -> %s\", Received: valid arena", arena.err, strerror(errno));
      assertm(arena.arena == arena_base_ptr,
              "Expected: Arena base ptr to remain at %p, Received: %p", arena_base_ptr, arena.arena);
      assertm(arena.size == arena_size, "Expected: Arena size to remain at %zu, Received: %zu", arena_size, arena.size);
      assertm(arena.offset == new_offset, "Expected: Arena offset to remain at %zu, Received: %zu", new_offset, arena.offset);
      /* now lets reset and attempt to allocate 2 bytes again */
      assertm(arena_reset(&arena), "Expected: arena reset to succeed, Received: false (%s)", arena.err);
      assertm(arena.offset == 0, "Expected: arena offset to be 0, Received: %zu", arena.offset);
      assertm(arena.err == NULL, "Expected: arena error to be reset to NULL, Received: %s", arena.err);
      c = arena_alloc(&arena, 2); // request 2 bytes of allocation
      (void) c;
      assertm(!arena.err,
              "Expected: allocation to work, Received: '%s -> %s'", arena.err, strerror(errno));
      assertm(arena.arena == arena_base_ptr,
              "Expected: Arena base ptr to remain at %p, Received: %p", arena_base_ptr, arena.arena);
      assertm(arena.size == arena_size, "Expected: Arena size to remain at %zu, Received: %zu", arena_size, arena.size);
      assertm(arena.offset == 2, "Expected: Arena offset to change to 2, Received: %zu", arena.offset);

      assertm(arena_free(&arena) && !arena.err,
              "Expected: arena free to work, Received: %s -> %s", arena.err,  strerror(errno));

      testlog(L_INFO, "[ARENA RESET HAPPY PATH TESTS] OK!");
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
              "Expected: %zu bytes to be zero filled, Received: %zu bytes were zero filled", calloced_bytes, zero_count);

      assertm(arena_free(&arena) && !arena.err,
              "Expected: arena free to work, Received: %s -> %s", arena.err,  strerror(errno));


      testlog(L_INFO, "[ARENA CALLOC HAPPY PATH TESTS] OK!");
    }

    {
      arena_t arena = arena_create(requested_arena_size);
      assertm(!arena.err, "Expected: valid arena to be created, Received: %s -> %s", arena.err, strerror(errno));

      /* test internal memory arena_alloc failure */
      char *c = arena_calloc(&arena, arena.size, 1);
      assertm(!arena.err, "Expected: %zu bytes to be allocated, Received: %p (%s -> %s)", arena.size, (void *)c, arena.err, strerror(errno));
      assertm(arena.offset == arena.size, "Expected: arena offset to be %zu, Received: %zu", arena.size, arena.offset);

      c = arena_calloc(&arena, 1, 1);
      assertm(c == NULL, "Expected: arena_calloc to fail as it should be full, Received: new ptr %p", (void *)c);
      assertm(arena.err, "Expected: arena to have an error %s -> %s, Received: %s", arena.err, strerror(errno), arena.err);

      assertm(arena_free(&arena) && !arena.err,
              "Expected: arena free to work, Received: %s -> %s", arena.err,  strerror(errno));

      testlog(L_INFO, "[ARENA CALLOC ERROR PATH TESTS] OK!");
    }
  }

  /* arena_realloc */
  {
    {
      arena_t arena = arena_create(requested_arena_size);
      assertm(!arena.err, "Expected: valid arena to be created, Received: %s -> %s", arena.err, strerror(errno));

      /* test old_sz == new_sz path */
      size_t len = arena.size / 2;
      char *c = arena_alloc(&arena, len);
      assertm(!arena.err, "Expected: %zu bytes to be allocated, Received: %p (%s -> %s)", len, (void *)c, arena.err, strerror(errno));
      assertm(arena.offset == len, "Expected: arena offset to be %zu, Received: %zu", len, arena.offset);

      for (size_t i = 0; i < len; i++) {
        c[i] = i + 1;
      }
      c[len - 1] = '\0';

      c = arena_realloc(&arena, (void *)c, len, len);
      assertm(!arena.err, "Expected: %zu bytes to be allocated, Received: %p (%s -> %s)", len, (void *)c, arena.err, strerror(errno));
      assertm(arena.offset == arena.size, "Expected: arena offset to be %zu, Received: %zu", arena.size, arena.offset);

      for (size_t i = 0; i < len - 1; i++) {
        assertm(c[i] == (char)(i + 1), "Expected: %c, Received: %c", (char)(i + 1), c[i]);
      }
      assertm(c[len - 1] == '\0', "Expected: '\\0', Received: %c", c[len - 1]);

      arena_reset(&arena);

      /* test old size > new_sz path */
      len = arena.size - 16;
      c = arena_alloc(&arena, len);
      assertm(!arena.err, "Expected: %zu bytes to be allocated, Received: %p (%s -> %s)", len, (void *)c, arena.err, strerror(errno));
      assertm(arena.offset == len, "Expected: arena offset to be %zu, Received: %zu", len, arena.offset);

      c = arena_realloc(&arena, (void *)c, len, 16);
      assertm(!arena.err, "Expected: arena_realloc to succeed, Received: arena to have an error %s -> %s", arena.err, strerror(errno));
      assertm(c != NULL, "Expected: valid allocated pointer, Received: %p", (void *)c);
      assertm(arena.offset == arena.size, "Expected: arena offset to be %zu, Received: %zu", arena.size, arena.offset);

      arena_reset(&arena);

      /* realloc NULL as ptr should return a valid ptr just like realloc */
      len = arena.size / 2;
      c = arena_realloc(&arena, NULL, 0, len);
      assertm(!arena.err, "Expected: arena_realloc to succeed, Received: arena to have an error %s -> %s", arena.err, strerror(errno));
      assertm(c != NULL, "Expected: valid ptr, Received: %p", (void *)c);
      // cuz len is half the size of arena
      assertm(arena.offset == len, "Expected: offset %zu, Received: offset %zu", len, arena.offset);

      arena_reset(&arena);

      assertm(arena_free(&arena) && !arena.err,
              "Expected: arena free to work, Received: %s -> %s", arena.err,  strerror(errno));

      testlog(L_INFO, "[ARENA REALLOC HAPPY PATH TESTS] OK!");
    }

    {
      arena_t arena = arena_create(requested_arena_size);
      assertm(!arena.err, "Expected: valid arena to be created, Received: %s -> %s", arena.err, strerror(errno));

      char a = 'a';
      char *c = &a;

      /* test invalid ptr path */
      c = arena_realloc(&arena, c, 1, 10);
      assertm(c == NULL, "Expected: arena_realloc to fail as ptr %p is not in arena, Received: new ptr %p", (void *)c, (void *)c);
      assertm(arena.err, "Expected: arena to have an error %s -> %s, Received: %s", arena.err, strerror(errno), arena.err);
      assertm(arena.offset == 0, "Expected: arena offset to be 0, Received: %zu", arena.offset);

      arena_reset(&arena);

      /* test invalid old size path */
      int *valid = arena_realloc(&arena, NULL, 0, sizeof(int) * 8);
      c = arena_realloc(&arena, valid, 0, 20); // this should fail as the pointer is not NULL but old size is invalid (0)
      assertm(c == NULL, "Expected: arena_realloc to fail due to old size being 0, Received: new ptr %p", (void *)c);
      assertm(arena.err, "Expected: arena to have an error %s -> %s, Received: %s", arena.err, strerror(errno), arena.err);
      assertm(arena.offset == sizeof(int) * 8, "Expected: arena offset to be 0, Received: %zu", arena.offset);

      arena_reset(&arena);

      /* test invalid (ptr + old size) path */
      size_t len = arena.size - 1024;
      c = arena_alloc(&arena, len);
      assertm(!arena.err, "Expected: %zu bytes to be allocated, Received: %p (%s -> %s)", len, (void *)c, arena.err, strerror(errno));
      assertm(arena.offset == len, "Expected: arena offset to be %zu, Received: %zu", len, arena.offset);

      c = arena_realloc(&arena, c, arena.size, 1);
      assertm(c == NULL, "Expected: arena_realloc to fail as arena should be full, Received: new ptr %p", (void *)c);
      assertm(arena.err, "Expected: arena to have an error %s -> %s, Received: %s", arena.err, strerror(errno), arena.err);
      assertm(arena.offset == len, "Expected: arena offset to be %zu, Received: %zu", len, arena.offset);

      arena_reset(&arena);

      /* test internal call to arena_alloc should fail path */
      len = arena.size - 16;
      c = arena_alloc(&arena, len);
      assertm(!arena.err, "Expected: %zu bytes to be allocated, Received: %p (%s -> %s)", len, (void *)c, arena.err, strerror(errno));
      assertm(arena.offset == len, "Expected: arena offset to be %zu, Received: %zu", len, arena.offset);

      c = arena_realloc(&arena, (void *)c, len, 17);
      assertm(c == NULL, "Expected: arena_realloc to fail as arena can't fit 17 bytes, Received: new ptr %p", (void *)c);
      assertm(arena.err, "Expected: arena to have an error %s -> %s, Received: %s", arena.err, strerror(errno), arena.err);
      assertm(arena.offset == len, "Expected: arena offset to be %zu, Received: %zu", len, arena.offset);

      assertm(arena_free(&arena) && !arena.err,
              "Expected: arena free to work, Received: %s -> %s", arena.err,  strerror(errno));

      testlog(L_INFO, "[ARENA REALLOC ERROR PATH TESTS] OK!");
    }
  }

  testlog(L_INFO, "<zdx_simple_arena_test> All ok!\n");
  return 0;
}

static void test_arena_alloc(arena_t *const arena, const size_t sz, const size_t expected_offset, const size_t expected_alignment)
{
  /* log(L_INFO, "[ARENA ALLOC TEST] size %zu, exp offset %zu, exp align %zu", sz, expected_offset, expected_alignment); */
  assertm(sz > 0, "Expected: test_arena_alloc called with size > 0, Received: %zu", sz);

  int8_t *n_bytes = arena_alloc(arena, sz);
  assertm(!arena->err, "Expected: arena_alloc to succeed, Received: %s", arena->err);
  assertm(n_bytes != NULL, "Expected: a non-NULL allocation, Received: %p", (void *)n_bytes);
  assertm(arena->offset == expected_offset, "Expected: %zu, Received: %zu", expected_offset, arena->offset);

  uint64_t n_bytes_alignment = (uintptr_t)n_bytes % expected_alignment;
  assertm(n_bytes_alignment == 0, "Expected: %p aligned to 4, Received: %"PRIu64,(void *)n_bytes, n_bytes_alignment);

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
