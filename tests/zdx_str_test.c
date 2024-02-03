#include <stdio.h>
#define SB_MIN_CAPACITY 1
#define SB_RESIZE_FACTOR 2
#define GB_INIT_LENGTH 1
#define GB_MIN_GAP_SIZE 2
#define ZDX_STR_IMPLEMENTATION
#include "../zdx_str.h"
#include "../zdx_util.h"


int main(void)
{
  /* const char a[] = {'a', 'b', '\0'}; */
  /* const int *const i= (int []){1,2,4}; */
  /* int p[] = {4,5,6}; */
  /* i = p; */
  /* printf("omg %s %d\n", a, i[0]); */
  /* i[0] = 10; */
  /* printf("omg %s %d\n", a, i[0]); */

  /* typedef struct { */
  /* int i; */
  /* float j; */
  /* } omg_t; */

  /* const omg_t x = {.i=10, .j=11.0f}; */
  /* const omg_t y = {.i=14, .j=15.0f}; */

  /* printf("x %#x %d %d y %#x\n", x, (*(int *)&x) == x.i, (*((float *)&x + 1)) == x.j,  y); */
  /* x = y; */
  /* x.i = 20; */

  // ---- START STRING BUILDER TESTS ----

  sb_t sb = {0};

  const char *a[] = { "four", "five", "six", "\n" };
  const char **b = a;
  const char *c = *a;
  const char d = **a;
  const char **e = (const char *[]){"omg", "nomg"};
  (void) b;
  (void) c;
  (void) d;
  (void) e;

  /* CONVERT THESE TO TESTS USING DejaGnu as all of these should trigger failures and crash the program */
  /* sb_concat(NULL, a); */
  /* sb_append(NULL, "one", "two", "three"); */
  /* sb_concat(&sb, NULL); */
  /* sb_append(&sb, NULL); */
  /* sb_concat(NULL, NULL); */
  /* sb_append(NULL, NULL); */
  /* sb_append_buf(NULL, NULL, 0); */
  /* sb_append_buf(&sb, NULL, 0); */
  /* sb_append_buf(NULL, (char []){'a'}, 0); */
  /* sb_append_buf(NULL, (char []){'a'}, 1); */
  /* sb_append_buf(NULL, NULL, 1); */
  /* sb_append_buf(&sb, NULL, 1); */
  /* END */

  const char *f = "three";
  sb_append(&sb, "one", "two", f);
  assertm(sb.length == 11, "Expected: 11, Received: %zu", sb.length);
  assertm(sb.capacity == 16, "Expected: 16, Received: %zu", sb.capacity);
  assertm(strcmp(sb.str, "onetwothree") == 0, "Expected: onetwothree, Received: %s", sb.str);

  sb_concat(&sb, a);
  assertm(sb.length == 23, "Expected: 23, Received: %zu", sb.length);
  assertm(sb.capacity == 32, "Expected: 32, Received: %zu", sb.capacity);
  assertm(strcmp(sb.str, "onetwothreefourfivesix\n") == 0, "Expected: onetwothreefourfivesix\n, Received: %s", sb.str);

  char buf_arr[] = {'a', 'b', 'c'};
  sb_append_buf(&sb, buf_arr, 3);
  assertm(sb.length == 26, "Expected: 26, Received: %zu", sb.length);
  assertm(sb.capacity == 32, "Expected: 32, Received: %zu", sb.capacity);
  assertm(strcmp(sb.str, "onetwothreefourfivesix\nabc") == 0, "Expected: onetwothreefourfivesix\\nabc, Received: %s", sb.str);

  char *buf_ptr = buf_arr;
  sb_append_buf(&sb, buf_ptr, 3);
  assertm(sb.length == 29, "Expected: 29, Received: %zu", sb.length);
  assertm(sb.capacity == 32, "Expected: 32, Received: %zu", sb.capacity);
  assertm(strcmp(sb.str, "onetwothreefourfivesix\nabcabc") == 0, "Expected: onetwothreefourfivesix\\nabcabc, Received: %s", sb.str);

  sb_append_buf(&sb, (char []){'1', '2', '3'}, 3);
  assertm(sb.length == 32, "Expected: 32, Received: %zu", sb.length);
  assertm(sb.capacity == 64, "Expected: 64, Received: %zu", sb.capacity);
  assertm(strcmp(sb.str, "onetwothreefourfivesix\nabcabc123") == 0, "Expected: onetwothreefourfivesix\\nabcabc123, Received: %s", sb.str);

  sb_deinit(&sb);

  assertm(sb.str == NULL, "After free(), items in string builder should be NULL ptr");
  assertm(sb.length == 0, "After free(), length in string builder should be 0");
  assertm(sb.capacity == 0, "After free(), capacity in string builder should be 0");

  // ---- END STRING BUILDER TESTS ----

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
  assertm(strcmp(buf_cstr, "cater") == 0, "Expected: \"\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 5, "Expected: 5, Received: %zu", gb.length);

  gb_move_cursor(&gb, 2);
  gb_insert_char(&gb, 'p');
  gb_move_cursor(&gb, 100000);
  gb_move_cursor(&gb, 0);
  buf_cstr = gb_buf_as_cstr(&gb);
  assertm(strcmp(buf_cstr, "caterp") == 0, "Expected: \"\", Received: %s", buf_cstr);
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
  assertm(strcmp(buf_cstr, "caterpillers") == 0, "Expected: \"\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 12, "Expected: 12, Received: %zu", gb.length);

  gb_move_cursor(&gb, -1);
  gb_move_cursor(&gb, -5000);
  gb_insert_char(&gb, '*');
  gb_move_cursor(&gb, 5000);
  gb_insert_char(&gb, '*');
  gb_move_cursor(&gb, -5000);
  buf_cstr = gb_buf_as_cstr(&gb);
  assertm(strcmp(buf_cstr, "*caterpillers*") == 0, "Expected: \"\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 14, "Expected: 14, Received: %zu", gb.length);

  gb_deinit(&gb);

  /* gb_buf_as_dbg_cstr test */

  gb_init(&gb);

  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "..") == 0, "Expected: \"\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 0, "Expected: 0, Received: %zu", gb.length);

  gb_insert_char(&gb, 'a');
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "a.") == 0, "Expected: \"\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 1, "Expected: 1, Received: %zu", gb.length);

  gb_insert_char(&gb, 'b');
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "ab") == 0, "Expected: \"\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 2, "Expected: 2, Received: %zu", gb.length);

  gb_insert_char(&gb, 'c');
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "abc.") == 0, "Expected: \"\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 3, "Expected: 3, Received: %zu", gb.length);

  gb_move_cursor(&gb, -3);
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, ".abc") == 0, "Expected: \"\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 3, "Expected: 3, Received: %zu", gb.length);

  gb_insert_char(&gb, '1');
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "1abc") == 0, "Expected: \"\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 4, "Expected: 4, Received: %zu", gb.length);

  gb_insert_char(&gb, '2');
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "12.abc") == 0, "Expected: \"\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 5, "Expected: 5, Received: %zu", gb.length);

  gb_insert_char(&gb, '3');
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "123abc") == 0, "Expected: \"\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 6, "Expected: 6, Received: %zu", gb.length);
  assertm(gb.gap_start_ == 3, "Expected: 3, Received: %zu", gb.gap_start_);
  assertm(gb.gap_end_ == 3, "Expected: 3, Received: %zu", gb.gap_end_);

  gb_move_cursor(&gb, 3);
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "123abc") == 0, "Expected: \"\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 6, "Expected: 6, Received: %zu", gb.length);
  assertm(gb.gap_start_ == 6, "Expected: 6, Received: %zu", gb.gap_start_);
  assertm(gb.gap_end_ == 6, "Expected: 6, Received: %zu", gb.gap_end_);

  gb_insert_char(&gb, 'd');
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "123abcd.") == 0, "Expected: \"\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 7, "Expected: 7, Received: %zu", gb.length);

  gb_move_cursor(&gb, -2000);
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, ".123abcd") == 0, "Expected: \"\", Received: %s", buf_cstr);
  free(buf_cstr);
  assertm(gb.length == 7, "Expected: 7, Received: %zu", gb.length);

  gb_insert_char(&gb, '0');
  buf_cstr = gb_buf_as_dbg_cstr(&gb);
  assertm(strcmp(buf_cstr, "0123abcd") == 0, "Expected: \"\", Received: %s", buf_cstr);
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

  gb_move_cursor(&gb, 1000);
  gb_insert_cstr(&gb, "!!");
  buf_cstr = gb_buf_as_cstr(&gb);
  assertm(strcmp(buf_cstr, "12345abcd!!") == 0, "Expected: \"12345abcd!!\", Received: %s", buf_cstr);
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

  // ---- END GAP BUFFER TESTS ----

  log(L_INFO, "<zdx_str_test> All ok!\n");

  return 0;
}
