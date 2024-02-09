#define GB_INIT_LENGTH 1
#define GB_MIN_GAP_SIZE 2
#define ZDX_GAP_BUFFER_IMPLEMENTATION
#include "../zdx_gap_buffer.h"
#include "../zdx_util.h"
#define ZDX_FILE_IMPLEMENTATION
#include "../zdx_file.h"
#define ZDX_STR_IMPLEMENTATION
#include "../zdx_str.h"

int main(void)
{
    // ---- START GAP BUFFER TESTS ----

  gb_t gb = {0};

  /* gb_insert_char test */

  gb_init(&gb);

  char *buf_cstr = gb_buf_as_cstr(&gb);
  assertm(strcmp(buf_cstr, "") == 0, "Expected: \"\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 0, "Expected: 0, Received: %zu", gb.length);

  gb_insert_char(&gb, 'c');
  gb_insert_char(&gb, 'a');
  gb_insert_char(&gb, 'e');
  gb_insert_char(&gb, 'r');
  gb_move_cursor(&gb, -2);
  gb_insert_char(&gb, 't');
  buf_cstr = gb_buf_as_cstr(&gb);
  assertm(strcmp(buf_cstr, "cater") == 0, "Expected: \"cater\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 5, "Expected: 5, Received: %zu", gb.length);

  gb_move_cursor(&gb, 2);
  gb_insert_char(&gb, 'p');
  gb_move_cursor(&gb, 100000);
  gb_move_cursor(&gb, 0);
  buf_cstr = gb_buf_as_cstr(&gb);
  assertm(strcmp(buf_cstr, "caterp") == 0, "Expected: \"caterp\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 6, "Expected: 6, Received: %zu", gb.length);

  gb_insert_char(&gb, 'l');
  gb_insert_char(&gb, 'r');
  gb_move_cursor(&gb, -2);
  gb_insert_char(&gb, 'i');
  gb_insert_char(&gb, 'l');
  gb_move_cursor(&gb, 1);
  gb_insert_char(&gb, 'e');
  gb_move_cursor(&gb, 5000);
  gb_insert_char(&gb, 's');
  buf_cstr = gb_buf_as_cstr(&gb);
  assertm(strcmp(buf_cstr, "caterpillers") == 0, "Expected: \"caterpillers\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 12, "Expected: 12, Received: %zu", gb.length);

  gb_move_cursor(&gb, -1);
  gb_move_cursor(&gb, -5000);
  gb_insert_char(&gb, '*');
  gb_move_cursor(&gb, 5000);
  gb_insert_char(&gb, '*');
  gb_move_cursor(&gb, -5000);
  buf_cstr = gb_buf_as_cstr(&gb);
  assertm(strcmp(buf_cstr, "*caterpillers*") == 0, "Expected: \"*caterpillers*\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 14, "Expected: 14, Received: %zu", gb.length);

  gb_deinit(&gb);

  /* gb_buf_as_dbg_cstr test */

  gb_init(&gb);

  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "..") == 0, "Expected: \"..\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 0, "Expected: 0, Received: %zu", gb.length);

  gb_insert_char(&gb, 'a');
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "a.") == 0, "Expected: \"a.\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 1, "Expected: 1, Received: %zu", gb.length);

  gb_insert_char(&gb, 'b');
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "ab") == 0, "Expected: \"ab\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 2, "Expected: 2, Received: %zu", gb.length);

  gb_insert_char(&gb, 'c');
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "abc.") == 0, "Expected: \"abc.\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 3, "Expected: 3, Received: %zu", gb.length);

  gb_move_cursor(&gb, -3);
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, ".abc") == 0, "Expected: \".abc\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 3, "Expected: 3, Received: %zu", gb.length);

  gb_insert_char(&gb, '1');
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "1abc") == 0, "Expected: \"1abc\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 4, "Expected: 4, Received: %zu", gb.length);

  gb_insert_char(&gb, '2');
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "12.abc") == 0, "Expected: \"12.abc\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 5, "Expected: 5, Received: %zu", gb.length);

  gb_insert_char(&gb, '3');
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "123abc") == 0, "Expected: \"123abc\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 6, "Expected: 6, Received: %zu", gb.length);
  assertm(gb.gap_start_ == 3, "Expected: 3, Received: %zu", gb.gap_start_);
  assertm(gb.gap_end_ == 3, "Expected: 3, Received: %zu", gb.gap_end_);

  gb_move_cursor(&gb, 3);
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "123abc") == 0, "Expected: \"123abc\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 6, "Expected: 6, Received: %zu", gb.length);
  assertm(gb.gap_start_ == 6, "Expected: 6, Received: %zu", gb.gap_start_);
  assertm(gb.gap_end_ == 6, "Expected: 6, Received: %zu", gb.gap_end_);

  gb_insert_char(&gb, 'd');
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "123abcd.") == 0, "Expected: \"123abcd.\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 7, "Expected: 7, Received: %zu", gb.length);

  gb_move_cursor(&gb, -2000);
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, ".123abcd") == 0, "Expected: \".123abcd\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 7, "Expected: 7, Received: %zu", gb.length);

  gb_insert_char(&gb, '0');
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "0123abcd") == 0, "Expected: \"0123abcd\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 8, "Expected: 8, Received: %zu", gb.length);

  gb_deinit(&gb);

  /* gb_insert_cstr test */
  const char *some_str = "abd";
  const char *some_other_str = "12345";

  gb_init(&gb);

  buf_cstr = gb_buf_as_cstr(&gb);
  assertm(strcmp(buf_cstr, "") == 0, "Expected: \"\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 0, "Expected: 0, Received: %zu", gb.length);

  gb_insert_cstr(&gb, some_str);
  buf_cstr = gb_buf_as_cstr(&gb);
  assertm(strcmp(buf_cstr, "abd") == 0, "Expected: \"abd\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 3, "Expected: 3, Received: %zu", gb.length);

  gb_move_cursor(&gb, -1);
  gb_insert_cstr(&gb, "c");
  buf_cstr = gb_buf_as_cstr(&gb);
  assertm(strcmp(buf_cstr, "abcd") == 0, "Expected: \"abcd\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 4, "Expected: 4, Received: %zu", gb.length);

  gb_move_cursor(&gb, -1000000);
  gb_insert_cstr(&gb, some_other_str);
  buf_cstr = gb_buf_as_cstr(&gb);
  assertm(strcmp(buf_cstr, "12345abcd") == 0, "Expected: \"12345abcd\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 9, "Expected: 9, Received: %zu", gb.length);

  gb_move_cursor(&gb, -1000);
  gb_move_cursor(&gb, 5);
  gb_insert_cstr(&gb, "!!");
  buf_cstr = gb_buf_as_cstr(&gb);
  assertm(strcmp(buf_cstr, "12345!!abcd") == 0, "Expected: \"12345!!abcd\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 11, "Expected: 11, Received: %zu", gb.length);

  gb_deinit(&gb);

  /* gb_delete_char test */

  gb_init(&gb);

  buf_cstr = gb_buf_as_cstr(&gb);
  assertm(strcmp(buf_cstr, "") == 0, "Expected: \"\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 0, "Expected: 0, Received: %zu", gb.length);

  gb_insert_cstr(&gb, "abcdefghij");
  buf_cstr = gb_buf_as_cstr(&gb);
  assertm(strcmp(buf_cstr, "abcdefghij") == 0, "Expected: \"abcdefghij\", Received: %s", buf_cstr);
  free(buf_cstr);
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "abcdefghij....") == 0, "Expected: \"abcdefghij....\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 10, "Expected: 10, Received: %zu", gb.length);

  gb_move_cursor(&gb, -5);
  size_t curr_cursor = gb_get_cursor(&gb);
  assertm(curr_cursor == 5, "Expected: 5, Received: %zu", curr_cursor);
  gb_delete_chars(&gb, 2);
  assertm(curr_cursor == 5, "Expected: 5, Received: %zu", curr_cursor);
  buf_cstr = gb_buf_as_cstr(&gb);
  assertm(strcmp(buf_cstr, "abcdehij") == 0, "Expected: \"abcdehij\", Received: %s", buf_cstr);
  free(buf_cstr);
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "abcde......hij") == 0, "Expected: \"abcde......hij\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 8, "Expected: 8, Received: %zu", gb.length);

  gb_delete_chars(&gb, -2);
  gb_delete_chars(&gb, 0); // noop
  gb_delete_chars(&gb, 0); // noop
  buf_cstr = gb_buf_as_cstr(&gb);
  assertm(strcmp(buf_cstr, "abchij") == 0, "Expected: \"abchij\", Received: %s", buf_cstr);
  free(buf_cstr);
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "abc........hij") == 0, "Expected: \"abc........hij\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 6, "Expected: 6, Received: %zu", gb.length);
  curr_cursor = gb_get_cursor(&gb);
  assertm(curr_cursor == 3, "Expected: 3, Received: %zu", curr_cursor);

  gb_deinit(&gb);

  /* gb_insert_buf tests */

  gb_init(&gb);

  char buf_arr2[] = {'l', 'i', 'n', 'e', ' ', '0', '\n'};
  size_t buf_arr2_sz = sizeof(buf_arr2)/sizeof(buf_arr2[0]);
  fl_content_t fc = fl_read_file_str("./tests/mocks/simple.txt", "r");

  assertm(fc.is_valid, "Expected: valid file contents read from disk, Received: Error: %s", fc.err_msg);
  gb_insert_buf(&gb, fc.contents, fc.size);
  buf_cstr = gb_buf_as_cstr(&gb);
  assertm(strcmp(buf_cstr, (char *)fc.contents) == 0, "Expected: %s, Received: %s", (char *)fc.contents, buf_cstr);
  free(buf_cstr);
  assertm(gb.length == fc.size, "Expected: %zu, Received: %zu", fc.size, gb.length);
  curr_cursor = gb_get_cursor(&gb);
  assertm(curr_cursor == fc.size, "Expected: %zu, Received: %zu", fc.size, curr_cursor);

  gb_move_cursor(&gb, -10000);
  gb_insert_buf(&gb, buf_arr2, buf_arr2_sz);
  buf_cstr = gb_buf_as_cstr(&gb);

  sb_t temp_sb = {0};
  sb_append(&temp_sb, "line 0\n", (char *)fc.contents);
  assertm(strcmp(buf_cstr, temp_sb.str) == 0, "Expected: %s, Received: %s", (char *)fc.contents, buf_cstr);
  free(buf_cstr);
  assertm(gb.length == (fc.size + buf_arr2_sz), "Expected: %zu, Received: %zu", (fc.size + buf_arr2_sz), gb.length);
  curr_cursor = gb_get_cursor(&gb);
  assertm(curr_cursor == buf_arr2_sz, "Expected: %zu, Received: %zu", buf_arr2_sz, curr_cursor);

  sb_deinit(&temp_sb);
  fc_deinit(&fc);
  gb_deinit(&gb);

  /* gb_copy_chars_as_cstr tests */

  gb_init(&gb);

  gb_insert_cstr(&gb, "hello, world!");
  char *copied = gb_copy_chars_as_cstr(&gb, 0);
  assertm(copied == NULL, "Expected: NULL, Received: %s", copied);
  copied = gb_copy_chars_as_cstr(&gb, 1000);
  assertm(copied == NULL, "Expected: NULL, Received: %s", copied);

  gb_move_cursor(&gb, -6);
  copied = gb_copy_chars_as_cstr(&gb, 6);
  assertm(strcmp(copied, "world!") == 0, "Expected: \"world!\", Received: %s", copied);
  free(copied);

  gb_move_cursor(&gb, -10000);
  copied = gb_copy_chars_as_cstr(&gb, 12);
  assertm(strcmp(copied, "hello, world") == 0, "Expected: \"hello, world\", Received: %s", copied);
  free(copied);

  copied = gb_copy_chars_as_cstr(&gb, 10000);
  assertm(strcmp(copied, "hello, world!") == 0, "Expected: \"hello, world!\", Received: %s", copied);
  free(copied);

  gb_move_cursor(&gb, 5);
  copied = gb_copy_chars_as_cstr(&gb, -5);
  assertm(strcmp(copied, "hello") == 0, "Expected: \"hello\", Received: %s", copied);
  free(copied);

  copied = gb_copy_chars_as_cstr(&gb, -500);
  assertm(strcmp(copied, "hello") == 0, "Expected: \"hello\", Received: %s", copied);
  free(copied);

  gb_move_cursor(&gb, -1);
  copied = gb_copy_chars_as_cstr(&gb, -5);
  assertm(strcmp(copied, "hell") == 0, "Expected: \"hell\", Received: %s", copied);
  free(copied);

  gb_move_cursor(&gb, -4);
  copied = gb_copy_chars_as_cstr(&gb, -0);
  assertm(copied == NULL, "Expected: \"NULL\", Received: %s", copied);
  free(copied);

  gb_move_cursor(&gb, 7);
  copied = gb_copy_chars_as_cstr(&gb, -5);
  assertm(strcmp(copied, "llo, ") == 0, "Expected: \"llo, \", Received: %s", copied);
  free(copied);

  gb_delete_chars(&gb, -7);
  copied = gb_copy_chars_as_cstr(&gb, -5);
  assertm(copied == NULL, "Expected: \"NULL\", Received: %s", copied);
  free(copied);

  copied = gb_copy_chars_as_cstr(&gb, 5);
  assertm(strcmp(copied, "world") == 0, "Expected: \"world\", Received: %s", copied);
  free(copied);

  gb_move_cursor(&gb, 7);
  copied = gb_copy_chars_as_cstr(&gb, -5);
  assertm(strcmp(copied, "orld!") == 0, "Expected: \"orld!\", Received: %s", copied);
  free(copied);

  gb_deinit(&gb);

  // ---- END GAP BUFFER TESTS ----

  log(L_INFO, "<zdx_gap_buffer_test> All ok!\n");
}
