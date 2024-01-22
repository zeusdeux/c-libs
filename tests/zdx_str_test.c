#include <stdio.h>
#define SB_MIN_CAPACITY 1
#define SB_RESIZE_FACTOR 2
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
  /* END */

  sb_append(&sb, "one", "two", "three");
  assertm(sb.length == 11, "Expected: 11, Received: %zu", sb.length);
  assertm(sb.capacity == 16, "Expected: 16, Received: %zu", sb.capacity);
  assertm(strcmp(sb.str, "onetwothree") == 0, "Expected: onetwothree, Received: %s", sb.str);

  sb_concat(&sb, a);
  assertm(sb.length == 23, "Expected: 23, Received: %zu", sb.length);
  assertm(sb.capacity == 32, "Expected: 32, Received: %zu", sb.capacity);
  assertm(strcmp(sb.str, "onetwothreefourfivesix\n") == 0, "Expected: onetwothreefourfivesix\n, Received: %s", sb.str);

  sb_free(&sb);

  /* assertm(0, "Nope"); */

  log(L_INFO, "<zdx_str_test> All ok!\n");
  return 0;
}
