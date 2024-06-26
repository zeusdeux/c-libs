#include "../zdx_test_utils.h"

#define ZDX_STRING_VIEW_IMPLEMENTATION
#include "../zdx_string_view.h"


int main(void)
{
  /* sv_from_buf */
  {
    const char buf[] = { 'a', 'b', 'c', 'd', 'f', '\t', 'g', 'h', '\n', 'i', 'j', 'k' };
    const char *buf_as_cstr = "abcdf\tgh\nijk";
    sv_t sv = sv_from_buf(buf, sizeof(buf));
    assertm(sv_eq_cstr(sv, buf_as_cstr), "Expected: \"%s\", Received: "SV_FMT, buf_as_cstr, sv_fmt_args(sv));
  }

  /* sv_begins_with_word_cstr */
  {
    sv_t sv = sv_from_cstr("");
    assertm(!sv_begins_with_word_cstr(sv, " const"), "Expected: false, Received: true");
    sv = sv_from_cstr("con");
    assertm(!sv_begins_with_word_cstr(sv, " const"), "Expected: false, Received: true");

    sv = sv_from_cstr("const char *str = \"omg\";");
    assertm(sv_begins_with_word_cstr(sv, "const"), "Expected: true, Received: false");
    assertm(!sv_begins_with_word_cstr(sv, " const"), "Expected: false, Received: true");
    assertm(!sv_begins_with_word_cstr(sv, "const "), "Expected: false, Received: true");

    sv = sv_from_cstr("const");
    assertm(sv_begins_with_word_cstr(sv, "const"), "Expected: true, Received: false");

    sv = sv_from_cstr("constantinople");
    assertm(!sv_begins_with_word_cstr(sv, "const"), "Expected: false, Received: true");
  }

  /* sv_trim_left */
  {
    sv_t sv = sv_from_cstr(" \n\r\n\t   hello\n\t  \r\n");
    char *expected = "hello\n\t  \r\n";
    assertm(sv_eq_cstr(sv_trim_left(sv), expected), "Expected: %s, Received: "SV_FMT, expected, sv_fmt_args(sv));
    assertm(!sv_eq_cstr(sv_trim_left(sv), "NOPE"), "Expected: string view to not match, Received: true");

    sv = sv_from_cstr("");
    expected = "";
    assertm(sv_eq_cstr(sv_trim_left(sv), expected), "Expected: %s, Received: "SV_FMT, expected, sv_fmt_args(sv));
    assertm(!sv_eq_cstr(sv_trim_left(sv), "NOPE"), "Expected: string view to not match, Received: true");
  }

  /* sv_trim_right */
  {
    sv_t sv = sv_from_cstr(" \n\r\n\t   hello\n\t  \r\n");
    const char *expected = " \n\r\n\t   hello";

    assertm(sv_eq_cstr(sv_trim_right(sv), expected), "Expected: %s, Received: "SV_FMT, expected, sv_fmt_args(sv));
    assertm(!sv_eq_cstr(sv_trim_right(sv), "NOPE"), "Expected: string view to not match, Received: true");

    sv = sv_from_cstr("");
    expected = "";
    assertm(sv_eq_cstr(sv_trim_right(sv), expected), "Expected: %s, Received: "SV_FMT, expected, sv_fmt_args(sv));
    assertm(!sv_eq_cstr(sv_trim_right(sv), "NOPE"), "Expected: string view to not match, Received: true");
  }

  /* sv_trim */
  {
    sv_t sv = sv_from_cstr(" \n\r\n\t   hello\n\t  \r\n");
    const char *expected = "hello";

    assertm(sv_eq_cstr(sv_trim(sv), expected), "Expected: %s, Received: "SV_FMT, expected, sv_fmt_args(sv));
    assertm(!sv_eq_cstr(sv_trim(sv), "NOPE"), "Expected: string view to not match, Received: true");

    sv = sv_from_cstr("");
    expected = "";
    assertm(sv_eq_cstr(sv_trim(sv), expected), "Expected: %s, Received: "SV_FMT, expected, sv_fmt_args(sv));
    assertm(!sv_eq_cstr(sv_trim(sv), "NOPE"), "Expected: string view to not match, Received: true");
  }

  /* sv_split_by_char */
  {
    const char *str = "hello, world,\nomg test";
    sv_t sv;
    sv_t chunk, expected_chunk;

    sv = sv_from_cstr(str);
    chunk = sv_split_by_char(&sv, '|');
    expected_chunk = sv_from_cstr(str);
    assertm(sv_eq_sv(chunk, expected_chunk), "Expected: "SV_FMT", Received: "SV_FMT, sv_fmt_args(expected_chunk), sv_fmt_args(chunk));
    assertm(sv_eq_cstr(chunk, str), "Expected: \"%s\", Received: "SV_FMT, str, sv_fmt_args(sv));

    sv = sv_from_cstr(str);
    chunk = sv_split_by_char(&sv, ',');
    expected_chunk = sv_from_cstr("hello");
    assertm(sv_eq_sv(chunk, expected_chunk), "Expected: "SV_FMT", Received: "SV_FMT, sv_fmt_args(expected_chunk), sv_fmt_args(chunk));
    assertm(sv_eq_cstr(sv, " world,\nomg test"), "Expected: \", world,\nomg test\", Received: "SV_FMT, sv_fmt_args(sv));

    sv = sv_from_cstr("");
    chunk = sv_split_by_char(&sv, ',');
    assertm(!chunk.buf, "Expected: buffer addr 0x0 len 0, Received: addr %p len %zu", (void *)chunk.buf, chunk.length);
    assertm(sv_eq_cstr(sv, ""), "Expected: \"\", Received: "SV_FMT, sv_fmt_args(sv));

    sv = sv_from_cstr("abc..123...000");
    const char *expected_chunks[] = {
      "abc",
        "",
        "123",
        "",
        "",
        "000"
    };
    expected_chunk = sv_split_by_char(&sv, '.');
    for (size_t i = 0; expected_chunk.buf; i++) {
      sv_t expected_sv = sv_from_cstr(expected_chunks[i]);

      // test both sv_split_by_char and sv_eq_cstr
      assertm(sv_eq_cstr(expected_chunk, expected_chunks[i]),
              "%zu) (sv_eq_cstr) Expected: '%s', Received: '"SV_FMT"' (buf %p len %zu)",
              i, expected_chunks[i], sv_fmt_args(expected_chunk), (void *)expected_chunk.buf, expected_chunk.length);

      // test both sv_split_by_char and sv_eq_sv
      assertm(sv_eq_sv(expected_chunk, expected_sv),
              "%zu) (sv_eq_sv) "
              "Expected: '"SV_FMT"' (buf %p len %zu 1st byte %d), Received: '"SV_FMT"' (buf %p len %zu 1st byte %d)",
              i,
              sv_fmt_args(expected_sv), (void *)expected_sv.buf, expected_sv.length, *expected_sv.buf,
              sv_fmt_args(expected_chunk), (void *)expected_chunk.buf, expected_chunk.length, *expected_chunk.buf);
      expected_chunk = sv_split_by_char(&sv, '.');
    }
  }

  /* sv_split_from_idx (includes idx) */
  {
    const char *str = "hello, world,\nomg test";
    sv_t sv = sv_from_cstr(str);
    sv_t chunk, expected_chunk;

    chunk = sv_split_from_idx(&sv, 100);
    expected_chunk = sv_from_cstr("");
    assertm(sv_eq_sv(chunk, expected_chunk), "Expected: "SV_FMT", Received: \""SV_FMT"\"", sv_fmt_args(expected_chunk), sv_fmt_args(chunk));
    assertm(sv_eq_cstr(sv, str), "Expected: \"%s\", Received: "SV_FMT, str, sv_fmt_args(sv));

    chunk = sv_split_from_idx(&sv, 7);
    expected_chunk = sv_from_cstr("world,\nomg test");
    assertm(sv_eq_sv(chunk, expected_chunk), "Expected: "SV_FMT", Received: "SV_FMT, sv_fmt_args(expected_chunk), sv_fmt_args(chunk));
    assertm(sv_eq_cstr(sv, "hello, "), "Expected: \"hello, \", Received: "SV_FMT, sv_fmt_args(sv));

    sv = sv_from_cstr("");
    chunk = sv_split_from_idx(&sv, 100);
    expected_chunk = sv_from_cstr("");
    assertm(sv_eq_sv(chunk, expected_chunk), "Expected: "SV_FMT", Received: "SV_FMT, sv_fmt_args(expected_chunk), sv_fmt_args(chunk));
    assertm(sv_eq_cstr(sv, ""), "Expected: \"\", Received: "SV_FMT, sv_fmt_args(sv));
  }

  /* sv_split_until_idx (excludes idx) */
  {
    const char *str = "hello, world,\nomg test";
    sv_t sv = sv_from_cstr(str);
    sv_t chunk, expected_chunk;

    chunk = sv_split_until_idx(&sv, 100);
    expected_chunk = sv_from_cstr("hello, world,\nomg test");
    assertm(sv_eq_sv(chunk, expected_chunk), "Expected: "SV_FMT", Received: \""SV_FMT"\"", sv_fmt_args(expected_chunk), sv_fmt_args(chunk));
    assertm(sv_eq_cstr(sv, ""), "Expected: \"%s\", Received: "SV_FMT, str, sv_fmt_args(sv));

    sv = sv_from_cstr(str);
    chunk = sv_split_until_idx(&sv, 14);
    expected_chunk = sv_from_cstr("hello, world,\n");
    assertm(sv_eq_sv(chunk, expected_chunk), "Expected: "SV_FMT", Received: "SV_FMT, sv_fmt_args(expected_chunk), sv_fmt_args(chunk));
    assertm(sv_eq_cstr(sv, "omg test"), "Expected: \"mg test\", Received: "SV_FMT, sv_fmt_args(sv));

    sv = sv_from_cstr("");
    chunk = sv_split_until_idx(&sv, 100);
    expected_chunk = sv_from_cstr("");
    assertm(sv_eq_sv(chunk, expected_chunk), "Expected: "SV_FMT", Received: "SV_FMT, sv_fmt_args(expected_chunk), sv_fmt_args(chunk));
    assertm(sv_eq_cstr(sv, ""), "Expected: \"\", Received: "SV_FMT, sv_fmt_args(sv));
  }

  testlog(L_INFO, "<zdx_string_view_test> All ok!\n");

  return 0;
}
