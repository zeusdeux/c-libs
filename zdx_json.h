#ifndef ZDX_JSON_H_
#define ZDX_JSON_H_

#include <stddef.h>

typedef struct json_t json_t;

typedef enum {
  JSON_NULL = 0,
  JSON_NUMBER,
  JSON_BOOLEAN,
  JSON_STRING,
  JSON_PARSE_ERROR,
} json_value_tag;

const char *json_type_cstr(json_value_tag jtag);
json_t json_parse(const char *const json_cstr, const size_t sz);

#endif // ZDX_JSON_H_

#ifdef ZDX_JSON_IMPLEMENTATION

#include <stdbool.h>
#include <ctype.h>

#include "./zdx_util.h"

#define ZDX_SIMPLE_ARENA_IMPLEMENTATION
#include "./zdx_simple_arena.h"

#define ZDX_STRING_VIEW_IMPLEMENTATION
#include "./zdx_string_view.h"

// TODO: setup a wrapper for arena_realloc and #define DA_REALLOC to it here
// You'll have to likely modify arena_realloc implementation to understand a
// special value such a -1 for old_sz to make it possible to even write a
// realloc() compatible wrapper around arena_realloc() for zdx_da.h to consume.
/* #define ZDX_DA_IMPLEMENTATION */
/* #include "./zdx_da.h" */

typedef enum {
  JSON_TOKEN_END = 0, // lexer keeps returning end token when done. Making this the first kind so that we can {0} init a json_token_t value
  JSON_TOKEN_WS,
  JSON_TOKEN_OCURLY,
  JSON_TOKEN_CCURLY,
  JSON_TOKEN_OSQR,
  JSON_TOKEN_CSQR,
  JSON_TOKEN_COLON,
  JSON_TOKEN_COMMA,
  JSON_TOKEN_SYMBOL, // null, true, false
  JSON_TOKEN_NUMBER, // int, float and sci notation aka 1e4, 1e+4, 1e-4, etc
  JSON_TOKEN_STRING,
  JSON_TOKEN_UNKNOWN
} json_token_kind_t;

typedef struct {
  json_token_kind_t kind;
  sv_t val;
} json_token_t;

typedef struct {
  size_t cursor;
  size_t line;
  size_t bol; // beginning of line. col = bol - cursor
  sv_t *input;
} json_lexer_t;

const char* json_token_kind_to_cstr(const json_token_kind_t kind)
{
  static const char *const token_kinds[] = {
    "JSON_TOKEN_END",
    "JSON_TOKEN_WS",
    "JSON_TOKEN_OCURLY",
    "JSON_TOKEN_CCURLY",
    "JSON_TOKEN_OSQR",
    "JSON_TOKEN_CSQR",
    "JSON_TOKEN_COLON",
    "JSON_TOKEN_COMMA",
    "JSON_TOKEN_SYMBOL",
    "JSON_TOKEN_NUMBER",
    "JSON_TOKEN_STRING",
    "JSON_TOKEN_UNKNOWN"
  };

  return token_kinds[kind];
}


json_token_t json_lexer_next_token(json_lexer_t *const lexer)
{
  json_token_t token = {0};

  // TODO: add assertions for values in lexer struct that make sense here

  if (lexer->input->length == 0 || lexer->cursor >= lexer->input->length) {
    token.kind = JSON_TOKEN_END;
    return token;
  }

  // TODO: store lexer->input->buf[lexer->cursor] in a variable ffs. This isn't straightforward
  // as we need the address of that to create a string view with and creating a variable will
  // create a copy on stack and hence change the address.
  // Try asserting &variable == &lexer->input->buf[lexer->cursor]

  if (isspace(lexer->input->buf[lexer->cursor])) {
    token.kind = JSON_TOKEN_WS;
    token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    if (lexer->input->buf[lexer->cursor] == '\n') {
      lexer->line += 1;
      lexer->bol = lexer->cursor + 1;
    }

    lexer->cursor += 1;
    return token;
  }

  if (lexer->input->buf[lexer->cursor] == '{') {
    token.kind = JSON_TOKEN_OCURLY;
    token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    lexer->cursor += 1;
    return token;
  }

  if (lexer->input->buf[lexer->cursor] == '}') {
    token.kind = JSON_TOKEN_CCURLY;
    token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    lexer->cursor += 1;
    return token;
  }

  if (lexer->input->buf[lexer->cursor] == '[') {
    token.kind = JSON_TOKEN_OSQR;
    token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    lexer->cursor += 1;
    return token;
  }

  if (lexer->input->buf[lexer->cursor] == ']') {
    token.kind = JSON_TOKEN_CSQR;
    token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    lexer->cursor += 1;
    return token;
  }

  if (lexer->input->buf[lexer->cursor] == ':') {
    token.kind = JSON_TOKEN_COLON;
    token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    lexer->cursor += 1;
    return token;
  }

  if (lexer->input->buf[lexer->cursor] == ',') {
    token.kind = JSON_TOKEN_COMMA;
    token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    lexer->cursor += 1;
    return token;
  }

  // null, true, false
  if (isalpha(lexer->input->buf[lexer->cursor])) {
    token.kind = JSON_TOKEN_SYMBOL;
    token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    lexer->cursor += 1;

    // ok to check rather than assert cursor < input.length since the already
    // consumed character is a valid token and hence we have something to return
    // even when this loop doesn't run.
    while(lexer->cursor < lexer->input->length && isalpha(lexer->input->buf[lexer->cursor])) {
      token.val.length += 1;
      lexer->cursor += 1;
    }
    return token;
  }

  // TODO: Asserts in lexing logic should actually return JSON_TOKEN_UNKNOWN with captured string view
  // and a note or error or reason added on to the json_token_t type with the string of why the current
  // JSON_TOKEN_UNKNOWN was generated so that parser can show useful errors. This assertm() stuff is only
  // during development of the lib to debug logic quickly. Also, we want to return a value rather than abort
  // as failing at lexing shouldn't fail the application that's using the lexer + parser ffs.
  // numbers
  int curr_char_isdigit = isdigit(lexer->input->buf[lexer->cursor]);
  int curr_char_isplus = lexer->input->buf[lexer->cursor] == '+';
  int curr_char_isminus = lexer->input->buf[lexer->cursor] == '-';

  if (curr_char_isdigit || curr_char_isplus || curr_char_isminus) {
    if (curr_char_isdigit) {
      token.kind = JSON_TOKEN_NUMBER;
      token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1); // consume <digit>

      lexer->cursor += 1; // move cursor past <digit>
    }
    else if (curr_char_isplus || curr_char_isminus) {
      assertm((lexer->cursor + 1) < lexer->input->length,
              "Reached end when expected digit at line %zu col %zu", lexer->line, (lexer->cursor + 1) - lexer->bol);
      assertm(isdigit(lexer->input->buf[lexer->cursor + 1]),
              "Expected a digit at line %zu col %zu but received '%c'",
              lexer->line, (lexer->cursor + 1) - lexer->bol, lexer->input->buf[lexer->cursor + 1]);

      token.kind = JSON_TOKEN_NUMBER;
      token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 2); // consume (<plus> | <minus>)<digit>

      lexer->cursor += 2; // move cursor past (<plus> | <minus>)<digit>
    }
    else {
      assertm(false, "THIS SHOULD BE UNREACHABLE");
    }

    while(lexer->cursor < lexer->input->length && isdigit(lexer->input->buf[lexer->cursor])) {
      token.val.length += 1;
      lexer->cursor += 1;
    }

    if (lexer->cursor < lexer->input->length && lexer->input->buf[lexer->cursor] == '.') {
      assertm((lexer->cursor + 1) < lexer->input->length,
              "Reached end when expected digit at line %zu col %zu", lexer->line, (lexer->cursor + 1) - lexer->bol);
      assertm(isdigit(lexer->input->buf[lexer->cursor + 1]),
              "Expected a digit at line %zu col %zu but received '%c'",
              lexer->line, (lexer->cursor + 1) - lexer->bol, lexer->input->buf[lexer->cursor + 1]);

      token.val.length += 2; // consume <dot><digit>
      lexer->cursor += 2; // move cursor past <dot><digit>

      while(lexer->cursor < lexer->input->length && isdigit(lexer->input->buf[lexer->cursor])) {
        token.val.length += 1;
        lexer->cursor += 1;
      }

      return token; // float
    }

    if (lexer->cursor < lexer->input->length && lexer->input->buf[lexer->cursor] == 'e') {
      assertm((lexer->cursor + 1) < lexer->input->length,
              "Reached end when expected '+', '-' or a digit at line %zu col %zu", lexer->line, (lexer->cursor + 1) - lexer->bol);

      int next_char_isdigit = isdigit(lexer->input->buf[lexer->cursor + 1]);
      int next_char_isplus = lexer->input->buf[lexer->cursor + 1] == '+';
      int next_char_isminus = lexer->input->buf[lexer->cursor + 1] == '-';

      if (next_char_isdigit) {
        token.val.length += 2; // consume <e><digit>
        lexer->cursor += 2; // move cursor past <e><digit>
      } else if (next_char_isplus || next_char_isminus) {
        assertm((lexer->cursor + 2) < lexer->input->length,
                "Reached end when expected a digit at line %zu col %zu", lexer->line, (lexer->cursor + 2) - lexer->bol);
        assertm(isdigit(lexer->input->buf[lexer->cursor + 2]),
                "Expected a digit at line %zu col %zu but received '%c'",
                lexer->line, (lexer->cursor + 2) - lexer->bol, lexer->input->buf[lexer->cursor + 2]);

        token.val.length += 3; // consume <e>(<plus> | <minus>)<digit>
        lexer->cursor += 3; // move cursor past <e>(<plus> | <minus>)<digit>
      } else {
        assertm(false, "Invalid scientific notation number at line %zu col %zu", lexer->line, (lexer->cursor + 1) - lexer->bol);
      }

      while (lexer->cursor < lexer->input->length && isdigit(lexer->input->buf[lexer->cursor])) {
        token.val.length += 1;
        lexer->cursor += 1;
      }

      return token; // scientific notation
    }

    return token; // int
  }

  // string
  if (lexer->input->buf[lexer->cursor] == '"') {
    token.kind = JSON_TOKEN_STRING;
    token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);

    lexer->cursor += 1;

    /* size_t no_of_esc_chars_in_succession = 0; */
    while(lexer->input->buf[lexer->cursor] != '"') {
      // TODO: handle esc chars in string
      /* char curr = lexer->input->buf[lexer->cursor]; */
      /* if (curr == '\') { */
      /* no_of_esc_chars_in_succession += 1; */
      /* double esc cancel out, for e.g,. '\\' */
      /* no_of_esc_chars_in_succession = no_of_esc_chars_in_succession % 2 == 0 ? 0 : no_of_esc_chars_in_succession; */
      /* } */
      token.val.length += 1;
      lexer->cursor += 1;
    }

    token.val.length += 1; // to swallow end '"'
    lexer->cursor +=1; // to move cursor past end '"'

    return token;
  }

  token.kind = JSON_TOKEN_UNKNOWN;
  token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);
  lexer->cursor += 1;

  return token;
}

typedef struct json_t {
  json_value_tag type;

  union {
    double number;
    bool boolean;
    char *string;
    char *parse_err;
  };
} json_t;

const char *json_type_cstr(json_value_tag jtag)
{
  static const char *const tag_names[] = {
    "json_null",
    "json_number",
    "json_boolean",
    "json_string",
    "json_parse_error"
  };

  return tag_names[jtag];
}

json_t json_parse(const char *const json, const size_t len)
{

  sv_t input = sv_trim(sv_from_buf(json, len));
  json_lexer_t lexer = {
    .input = &input
  };
  json_token_t tok = json_lexer_next_token(&lexer);

  while(tok.kind != JSON_TOKEN_END) {
    // TODO
    tok = json_lexer_next_token(&lexer);
  }

  arena_t ar = arena_create(len * sizeof(json_t));
  // TODO: parsing
  arena_free(&ar);
  return (json_t){0};
}

#endif // ZDX_JSON_IMPLEMENTATION
