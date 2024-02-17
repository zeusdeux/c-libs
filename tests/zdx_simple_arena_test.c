#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "../zdx_util.h"
#define ZDX_SIMPLE_ARENA_IMPLEMENTATION
#include "../zdx_simple_arena.h"

int main(void)
{
  size_t requested_arena_size = 4098;
  arena_t arena = arena_create(requested_arena_size);

  /* arena_create */
  {
    if (!arena.is_valid) {
      log(L_ERROR, "Arena creation error: %s", strerror(errno));
    }
    assertm(arena.is_valid, "Expected: valid arena to be created, Received: false");
    assertm(arena.arena != NULL, "Expected: non-NULL arena addr, Received: %p", arena.arena);
    size_t expected_arena_size = round_up_to_page_size_(requested_arena_size);
    assertm(arena.size == expected_arena_size, "Expected: %ld, Received: %zu", expected_arena_size, arena.size);
    assertm(arena.offset == 0, "Expected: 0, Received: %zu", arena.offset);
  }

  /* arena_free */
  {
    if (!arena_free(&arena)) {
      log(L_ERROR, "Arena free error: %s", strerror(errno));
    }
    assertm(!arena.is_valid, "Expected: arena to be invalidated on free, Received: true");
    assertm(arena.arena == NULL, "Expected: non-NULL arena addr, Received: %p", arena.arena);
    assertm(arena.size == 0, "Expected: 0, Received: %zu", arena.size);
    assertm(arena.offset == 0, "Expected: 0, Received: %zu", arena.offset);
  }

  log(L_INFO, "<zdx_gap_buffer_test> All ok!\n");
  return 0;
}
