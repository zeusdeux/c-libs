/**
 * MIT License
 *
 * Copyright (c) 2024 Mudit
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#ifndef ZDX_JSON_H_
#define ZDX_JSON_H_

#include <stddef.h>
#include <stdbool.h>
#include "./zdx_simple_arena.h"

typedef enum {
  JSON_VALUE_UNEXPECTED = 0,
  JSON_VALUE_NULL,
  JSON_VALUE_NUMBER,
  JSON_VALUE_BOOLEAN,
  JSON_VALUE_STRING,
  JSON_VALUE_ARRAY,
} json_value_kind_t;

typedef struct json_value_t {
  json_value_kind_t kind;
  union {
    const void *null;
    double number;
    bool boolean;
    char *string;
    struct {
      size_t length;
      size_t capacity;
      struct json_value_t *items;
    } array;
  };
  const char *err;
} json_value_t;

/* PARSER */
json_value_t json_parse(arena_t *const arena, const char *const json_cstr);

#endif // ZDX_JSON_H_

#ifdef ZDX_JSON_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "./zdx_util.h"
#define ZDX_STRING_VIEW_IMPLEMENTATION
#include "./zdx_string_view.h"

typedef enum {
  // err member will carry error string (static with no interpolation of row/col. That's the parser's job to build using the err str)
  // this is also what zero-initializing json_token_t sets the token.kind to. Sane default imho.
  JSON_TOKEN_UNKNOWN = 0,
  // lexer keeps returning end token when done. Making this the first kind so that we can {0} init a json_token_t value
  JSON_TOKEN_END,
  JSON_TOKEN_WS,
  JSON_TOKEN_OCURLY,
  JSON_TOKEN_CCURLY,
  JSON_TOKEN_OSQR,
  JSON_TOKEN_CSQR,
  JSON_TOKEN_COLON,
  JSON_TOKEN_COMMA,
  JSON_TOKEN_SYMBOL, // null, true, false
  // number
  JSON_TOKEN_LONG, // long int and long int in sci notation aka 1e4, 1e+4, 1e-4, etc
  JSON_TOKEN_DOUBLE, // floating point and floating point in sci notation aka 1e4, 1e+4, 1e-4, etc

  JSON_TOKEN_STRING,
} json_token_kind_t;

typedef struct {
  json_token_kind_t kind;
  sv_t val;
  const char *err;
} json_token_t;

typedef struct {
  size_t cursor;
  size_t line;
  size_t bol; // beginning of line. col = bol - cursor
  sv_t *input;
} json_lexer_t;

static const char* json_token_kind_to_cstr(const json_token_kind_t kind)
{
  static const char *const token_kinds[] = {
    "JSON_TOKEN_UNKNOWN",
    "JSON_TOKEN_END",
    "JSON_TOKEN_WS",
    "JSON_TOKEN_OCURLY",
    "JSON_TOKEN_CCURLY",
    "JSON_TOKEN_OSQR",
    "JSON_TOKEN_CSQR",
    "JSON_TOKEN_COLON",
    "JSON_TOKEN_COMMA",
    "JSON_TOKEN_SYMBOL",
    "JSON_TOKEN_LONG",
    "JSON_TOKEN_DOUBLE",
    "JSON_TOKEN_STRING",
  };

  return token_kinds[kind];
}

static json_token_t json_lexer_next_token(json_lexer_t *const lexer)
{
  json_token_t token = {0};

  // TODO: add assertions for values in lexer struct that make sense here

  if (lexer->input->length == 0 || lexer->cursor >= lexer->input->length) {
    token.kind = JSON_TOKEN_END;
    return token;
  }

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

  // Instead of asserts we instead return JSON_TOKEN_UNKNOWN with captured unknown chars in string view so that
  // parser can show useful errors. This assertm() stuff is only during development of the lib to debug logic quickly.
  // We really want to return a value rather than abort as failing at lexing shouldn't fail the application that's
  // using the lexer + parser.
  // numbers
  int curr_char_isdigit = isdigit(lexer->input->buf[lexer->cursor]);
  int curr_char_isplus = lexer->input->buf[lexer->cursor] == '+';
  int curr_char_isminus = lexer->input->buf[lexer->cursor] == '-';

  if (curr_char_isdigit || curr_char_isplus || curr_char_isminus) {
    if (curr_char_isdigit) {
      token.kind = JSON_TOKEN_LONG;
      token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1); // consume <digit>

      lexer->cursor += 1; // move cursor past <digit>
    }
    else if (curr_char_isplus || curr_char_isminus) {
      if ((lexer->cursor + 1) >= lexer->input->length || !isdigit(lexer->input->buf[lexer->cursor + 1])) {
        token.kind = JSON_TOKEN_UNKNOWN;
        token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1); // consume (<plus> | <minus>)
        token.err = "Expected a digit to follow";

        lexer->cursor += 1; // move cursor past (<plus> | <minus>)
        return token;
      }

      /* assertm((lexer->cursor + 1) < lexer->input->length, */
              /* "Reached end when expected digit at line %zu col %zu", lexer->line, (lexer->cursor + 1) - lexer->bol); */
      /* assertm(isdigit(lexer->input->buf[lexer->cursor + 1]), */
              /* "Expected a digit at line %zu col %zu but received '%c'", */
              /* lexer->line, (lexer->cursor + 1) - lexer->bol, lexer->input->buf[lexer->cursor + 1]); */

      token.kind = JSON_TOKEN_LONG;
      token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 2); // consume (<plus> | <minus>)<digit>
                                                                     //
      lexer->cursor += 2; // move cursor past (<plus> | <minus>)<digit>
    }
    else {
      assertm(false, "JSON LEXER: UNREACHABLE: Expected number to start with digit, '+' or '-'");
    }

    // TODO: due to this loop, we even lex "00.2313" as double which should technically fail parsing
    while(lexer->cursor < lexer->input->length && isdigit(lexer->input->buf[lexer->cursor])) {
      token.val.length += 1;
      lexer->cursor += 1;
    }

    // floating point component
    if (lexer->cursor < lexer->input->length && lexer->input->buf[lexer->cursor] == '.') {
      /* assertm((lexer->cursor + 1) < lexer->input->length, */
      /*         "Reached end when expected digit at line %zu col %zu", lexer->line, (lexer->cursor + 1) - lexer->bol); */
      /* assertm(isdigit(lexer->input->buf[lexer->cursor + 1]), */
      /*         "Expected a digit at line %zu col %zu but received '%c'", */
      /*         lexer->line, (lexer->cursor + 1) - lexer->bol, lexer->input->buf[lexer->cursor + 1]); */

      if ((lexer->cursor + 1) >= lexer->input->length || !isdigit(lexer->input->buf[lexer->cursor + 1])) {
        token.kind = JSON_TOKEN_UNKNOWN;
        token.val.length += 1; // consume <dot>
        token.err = "Expected a digit to follow";

        lexer->cursor += 1; // move cursor past <dot>
        return token;
      }

      token.kind = JSON_TOKEN_DOUBLE;
      token.val.length += 2; // consume <dot><digit>
      lexer->cursor += 2; // move cursor past <dot><digit>

      while(lexer->cursor < lexer->input->length && isdigit(lexer->input->buf[lexer->cursor])) {
        token.val.length += 1;
        lexer->cursor += 1;
      }
    }

    // exponent component
    if (lexer->cursor < lexer->input->length && (lexer->input->buf[lexer->cursor] == 'e' || lexer->input->buf[lexer->cursor] == 'E')) {
      /* assertm((lexer->cursor + 1) < lexer->input->length, */
      /*         "Reached end when expected '+', '-' or a digit at line %zu col %zu", lexer->line, (lexer->cursor + 1) - lexer->bol); */

      if ((lexer->cursor + 1) >= lexer->input->length) {
          token.kind = JSON_TOKEN_UNKNOWN;
          token.val.length += 1; // consume (<e> | <E>)
          token.err = "Expected a '+', '-' or a digit to follow";

          lexer->cursor += 1; // move cursor past (<e> | <E>)
          return token;
        }


      int next_char_isdigit = isdigit(lexer->input->buf[lexer->cursor + 1]);
      int next_char_isplus = lexer->input->buf[lexer->cursor + 1] == '+';
      int next_char_isminus = lexer->input->buf[lexer->cursor + 1] == '-';

      if (next_char_isdigit) {
        token.val.length += 2; // consume <e><digit>
        lexer->cursor += 2; // move cursor past <e><digit>
      }
      else if (next_char_isplus || next_char_isminus) {
        /* assertm((lexer->cursor + 2) < lexer->input->length, */
        /*         "Reached end when expected a digit at line %zu col %zu", lexer->line, (lexer->cursor + 2) - lexer->bol); */
        /* assertm(isdigit(lexer->input->buf[lexer->cursor + 2]), */
        /*         "Expected a digit at line %zu col %zu but received '%c'", */
        /*         lexer->line, (lexer->cursor + 2) - lexer->bol, lexer->input->buf[lexer->cursor + 2]); */

        if ((lexer->cursor + 2) >= lexer->input->length || !isdigit(lexer->input->buf[lexer->cursor + 2])) {

          token.kind = JSON_TOKEN_UNKNOWN;
          token.val.length += 2; // consume (<e> | <E>)(<plus> | <minus>)
          token.err = "Expected a digit to follow";

          lexer->cursor += 2; // move cursor past (<e> | <E>)(<plus> | <minus>)
          return token;
        }

        token.val.length += 3; // consume <e>(<plus> | <minus>)<digit>
        lexer->cursor += 3; // move cursor past <e>(<plus> | <minus>)<digit>
      }
      else {
        /* assertm(false, "Invalid scientific notation number at line %zu col %zu", lexer->line, (lexer->cursor + 1) - lexer->bol); */
        token.kind = JSON_TOKEN_UNKNOWN;
        token.val.length += 1;
        token.err = "Expected a '+', '-' or a digit to follow";

        lexer->cursor += 1; // move cursor past (<e> | <E>)
        return token;
      }

      while (lexer->cursor < lexer->input->length && isdigit(lexer->input->buf[lexer->cursor])) {
        token.val.length += 1;
        lexer->cursor += 1;
      }
    }

    return token; // valid LONG or DOUBLE token
  }

  // string
  if (lexer->input->buf[lexer->cursor] == '"') {
    token.kind = JSON_TOKEN_STRING;
    token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1); // consume OPENING <dbl_quote>
    lexer->cursor += 1; // move cursor past OPENING <dbl_quote>

    while(true) {
      // end of input before end of string so return JSON_TOKEN_UNKNOWN
      if (lexer->cursor >= lexer->input->length) {
        token.kind = JSON_TOKEN_UNKNOWN;
        token.err = "Expected a closing quote (\")";
        return token;
      }

      if (lexer->input->buf[lexer->cursor] == '"' && lexer->input->buf[lexer->cursor - 1] != '\\') {
        token.val.length += 1; // consume CLOSING <dbl_quote>
        token.kind = JSON_TOKEN_STRING;

        lexer->cursor += 1; // move cursor past CLOSING <dbl_quote>
        return token;
      }

      token.val.length += 1;
      lexer->cursor += 1;
    }

    assertm(false, "JSON LEXER: UNREACHABLE: Expected JSON_TOKEN_STRING or JSON_TOKEN_UNKNOWN to have already been returned");
  }

  token.kind = JSON_TOKEN_UNKNOWN; // this is default but I'm hardcoding just in case I re-order the json_token_type_t enum again
  token.val = sv_from_buf(&lexer->input->buf[lexer->cursor], 1);
  token.err = "Unrecognised token";

  lexer->cursor += 1;
  return token;
}

static json_token_t json_lexer_peek_token(json_lexer_t *const lexer)
{
  json_lexer_t before = {
    .cursor = lexer->cursor,
    .line = lexer->line,
    .bol = lexer->bol,
    .input = lexer->input
  };

  json_token_t tok = json_lexer_next_token(lexer);

  /* printf("<< kind %s \t| val '"SV_FMT"'\n", json_token_kind_to_cstr(tok.kind), sv_fmt_args(tok.val)); */

  lexer->cursor = before.cursor;
  lexer->line = before.line;
  lexer->bol = before.bol;
  lexer->input = before.input;

  return tok;
}
static const char *json_value_kind_to_cstr(json_value_kind_t kind)
{
  static const char *const value_kinds[] = {
    "JSON_VALUE_UNEXPECTED",
    "JSON_VALUE_NULL",
    "JSON_VALUE_NUMBER",
    "JSON_VALUE_BOOLEAN",
    "JSON_VALUE_STRING",
    "JSON_VALUE_ARRAY",
  };

  return value_kinds[kind];
}

static json_value_t json_build_unexpected(arena_t *const arena, json_lexer_t *const lexer, json_token_t tok, const char *const err_prefix)
{

  /* printf(">> kind %s \t| val '"SV_FMT"'\n", json_token_kind_to_cstr(tok.kind), sv_fmt_args(tok.val)); */
  json_value_t jv = {0};

  assertm(err_prefix && strlen(err_prefix) > 0,
          "Expected an error message but received %s (%p)", err_prefix, (void *)err_prefix);

  size_t sz = strlen(err_prefix) + 128;
  char *err = arena_calloc(arena, 1, sz);

  snprintf(err, sz, "%s at line %zu and col %zu",
           err_prefix,
           lexer->line + 1 /* line is 0 index */,
           lexer->cursor - lexer->bol - tok.val.length + 1 /* +1 for managing 0 index */);

  jv.kind = JSON_VALUE_UNEXPECTED;
  jv.err = err;

  return jv;
}

static json_value_t json_parse_symbol(arena_t *const arena, json_lexer_t *const lexer)
{
  json_value_t jv = {0};
  json_token_t tok = json_lexer_next_token(lexer);

  assertm(tok.kind == JSON_TOKEN_SYMBOL, "Expected a symbol token but received %s", json_token_kind_to_cstr(tok.kind));

  if (sv_eq_cstr(tok.val, "null")) {
    jv.kind = JSON_VALUE_NULL;
    jv.null = NULL;
  }
  else if (sv_eq_cstr(tok.val, "true")) {
    jv.kind = JSON_VALUE_BOOLEAN;
    jv.boolean = true;
  }
  else if (sv_eq_cstr(tok.val, "false")) {
    jv.kind = JSON_VALUE_BOOLEAN;
    jv.boolean = false;
  }
  else {
    const char* err_prefix = "Invalid symbol";
    size_t sz = strlen(err_prefix) + tok.val.length + 16; // 16 bytes are YOLO padding just in case
    char *err = arena_calloc(arena, 1, sz);

    snprintf(err, sz, "%s '"SV_FMT"'",
             err_prefix,
             sv_fmt_args(tok.val));

    jv = json_build_unexpected(arena, lexer, tok, err);
  }

  return jv;
}

static json_value_t json_parse_number(arena_t *const arena, json_lexer_t *const lexer)
{
  json_value_t jv = {0};
  json_token_t tok = json_lexer_next_token(lexer);

  assertm(tok.kind == JSON_TOKEN_LONG || tok.kind == JSON_TOKEN_DOUBLE,
          "Expected a long or double token but received %s", json_token_kind_to_cstr(tok.kind));

  char *str = arena_calloc(arena, 1, tok.val.length + 1);
  memcpy(str, tok.val.buf, tok.val.length);

  /* if (tok.kind == JSON_TOKEN_LONG) { */
    /* jv.num_long = strtoll(str, NULL, 10); */
  /* } */

  /* if (tok.kind == JSON_TOKEN_DOUBLE) { */
    /* jv.num_double = strtod(str, NULL); */
  /* } */

  // TODO: ERROR HANDLING
  jv.number = strtod(str, NULL); // strtoll doesn't parse exponent form hence using only strtod which does
  jv.kind = JSON_VALUE_NUMBER;

  return jv;
}

// TODO: handle unicode and other escapes that expand to strings
static json_value_t json_parse_string(arena_t *const arena, json_lexer_t *const lexer)
{
  json_value_t jv = {0};
  json_token_t tok = json_lexer_next_token(lexer);

  assertm(tok.kind == JSON_TOKEN_STRING, "Expected a string token but received %s", json_token_kind_to_cstr(tok.kind));

  char *str = arena_calloc(arena, 1, tok.val.length);

  /**
   * tok.val.buf + 1 to remove leading dbq qt (")
   * tok.val.length - 2 to remove trailing dbl qt (") and \0 from original tok.val contents
   * Also, we don't add a \0 at the end as we use arena_calloc and thus init whole string
   * with \0 before memcpy-ing
   **/
  memcpy(str, tok.val.buf + 1, tok.val.length - 2);

  jv.kind = JSON_VALUE_STRING;
  jv.string = str;

  return jv;
}

static json_value_t json_parse_unknown(arena_t *const arena, json_lexer_t *const lexer)
{
  json_token_t tok = json_lexer_next_token(lexer);

  assertm(tok.kind == JSON_TOKEN_UNKNOWN, "Expected an unknown token but received %s", json_token_kind_to_cstr(tok.kind));
  assertm(tok.err, "Expected unknown token to carry an error but received %s (%p)", tok.err, (void *)tok.err);

  size_t sz = strlen(tok.err) + tok.val.length + 16; // 16 bytes of padding just in case we run out of space
  char *err = arena_calloc(arena, 1, sz);

  snprintf(err, sz, "%s '"SV_FMT"'",
           tok.err,
           sv_fmt_args(tok.val));

  return json_build_unexpected(arena, lexer, tok, err);
}

#define JSON_DA_MIN_CAP 256
#define json_da_push(arena, arr, item) {                                          \
    while(((arr)->length + 1) > (arr)->capacity) {                                \
      size_t new_sz = (arr)->capacity ? ((arr)->capacity * 2) : JSON_DA_MIN_CAP;  \
      (arr)->items = arena_realloc((arena), (arr)->items, (arr)->length, new_sz); \
      (arr)->capacity = new_sz;                                                   \
    }                                                                             \
    (arr)->items[(arr)->length++] = (item);                                       \
  } while(0)

#define JSON_MAX_ARRAY_DEPTH 256

static json_value_t json_parse_array(arena_t *const arena, json_lexer_t *const lexer)
{
  json_value_t jvs[JSON_MAX_ARRAY_DEPTH] = {0};
  json_token_t tok = json_lexer_next_token(lexer);
  (void) arena;
  (void) jvs;

  assertm(tok.kind == JSON_TOKEN_OSQR, "Expected an opening '[' but received %s", json_token_kind_to_cstr(tok.kind));
  assertm(false, "TODO: Implement");

  while(true) {
  }

}

static json_value_t json_parse_object(arena_t *const arena, json_lexer_t *const lexer)
{
  (void) arena;
  (void) lexer;
  assertm(false, "TODO: Implement");
}

// TODO: HANDLE ERRORS ALL ACROSS THIS CODE
json_value_t json_parse(arena_t *const arena, const char *const json_cstr)
{
  json_value_t jv = {0};
  sv_t input = sv_from_cstr(json_cstr);
  json_lexer_t lexer = {
    .input = &input
  };

  // get token without changing lexer state
  // as it's on the json_parse_* functions to
  // consume the token and move the lexer forward
  json_token_t tok = json_lexer_peek_token(&lexer);

  // ignore leading whitespace
  while(tok.kind == JSON_TOKEN_WS) {
    json_lexer_next_token(&lexer); // move lexer forward
    // get first token without changing lexer state aka don't consume token
    // as the json_parse_* function below do that
    tok = json_lexer_peek_token(&lexer);
  }

  switch (tok.kind) {
  case JSON_TOKEN_SYMBOL: {
    jv = json_parse_symbol(arena, &lexer);
  } break;

  case JSON_TOKEN_LONG:
  case JSON_TOKEN_DOUBLE: {
    jv = json_parse_number(arena, &lexer);
  } break;

  case JSON_TOKEN_STRING: {
    jv = json_parse_string(arena, &lexer);
  } break;

  case JSON_TOKEN_UNKNOWN: {
    jv = json_parse_unknown(arena, &lexer);
  } break;

  case JSON_TOKEN_OSQR: {
    jv = json_parse_array(arena, &lexer);
  } break;

  case JSON_TOKEN_OCURLY: {
    jv = json_parse_object(arena, &lexer);
  } break;

  case JSON_TOKEN_END: return json_build_unexpected(arena, &lexer, tok, "Unexpected end of input");

  default: {
    assertm(false, "JSON PARSER: UNREACHABLE: Cannot parse unknown token kind '"SV_FMT"'", sv_fmt_args(tok.val));
  } break;
  }

  // consume next token as no more json_parse_* calls below
  tok = json_lexer_next_token(&lexer);

  // ignore trailing whitespace
  while(tok.kind == JSON_TOKEN_WS) {
    tok = json_lexer_next_token(&lexer); // consume whitespace token as no more json_parse_* calls below
  }

  if (tok.kind != JSON_TOKEN_END) {
    const size_t sz = 128;
    char *err = arena_calloc(arena, 1, sz);

    snprintf(err, sz, "Expected end of input but received '"SV_FMT"' (%s)",
             sv_fmt_args(tok.val),
             json_token_kind_to_cstr(tok.kind));

    return json_build_unexpected(arena, &lexer, tok, err);
  }

  return jv;
}
#endif // ZDX_JSON_IMPLEMENTATION
